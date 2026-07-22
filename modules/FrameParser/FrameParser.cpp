#include "FrameParser.hpp"
#include "FrameDecoder.hpp"
#include <cstring>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "RadioInterface.hpp"

static const char* Tag = "FrameParser";
static uint8_t TargetBssid[6];
static SearchMode ActiveMode = static_cast<SearchMode>(-1);

static void OnDataFrame(void*, esp_event_base_t, int32_t, void* EventData) {
    auto* Frame = static_cast<wifi_promiscuous_pkt_t*>(EventData);
    if (!FrameDecoder::BssidMatches(Frame, TargetBssid)) return;

    EapolPacket* Eapol = FrameDecoder::ExtractEapol(reinterpret_cast<DataFrame*>(Frame->payload));
    if (!Eapol) return;

    EapolKeyPacket* EapolKey = FrameDecoder::ExtractEapolKey(Eapol);
    if (!EapolKey) return;

    if (ActiveMode == SearchHandshake) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
            FrameParserEvents, EventEapolKeyFrame,
            Frame,
            sizeof(wifi_promiscuous_pkt_t) + Frame->rx_ctrl.sig_len,
            portMAX_DELAY));
        return;
    }

    if (ActiveMode == SearchPmkid) {
        PmkidNode* Nodes = FrameDecoder::ExtractPmkid(EapolKey);
        if (!Nodes) return;
        ESP_ERROR_CHECK(esp_event_post(
            FrameParserEvents, EventPmkidCaptured,
            &Nodes, sizeof(PmkidNode*), portMAX_DELAY));
    }
}

namespace FrameParser {
    void StartCapture(SearchMode Mode, const uint8_t* Bssid) {
        ActiveMode = Mode;
        memcpy(TargetBssid, Bssid, 6);
        ESP_ERROR_CHECK(esp_event_handler_register(
            RadioScannerEvents, EventPacketData, &OnDataFrame, nullptr));
    }

    void StopCapture() {
        ESP_ERROR_CHECK(esp_event_handler_unregister(
            ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &OnDataFrame));
    }
}
