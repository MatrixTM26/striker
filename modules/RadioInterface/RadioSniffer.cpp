#include "RadioInterface.hpp"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

ESP_EVENT_DEFINE_BASE(RadioScannerEvents);

static const char* Tag = "RadioSniffer";

static void OnRawFrame(void* Buf, wifi_promiscuous_pkt_type_t Type) {
    auto* Frame = static_cast<wifi_promiscuous_pkt_t*>(Buf);
    int32_t EvId;
    switch (Type) {
        case WIFI_PKT_DATA: EvId = EventPacketData;       break;
        case WIFI_PKT_MGMT: EvId = EventPacketManagement; break;
        case WIFI_PKT_CTRL: EvId = EventPacketControl;    break;
        default: return;
    }
    ESP_ERROR_CHECK(esp_event_post(
        RadioScannerEvents, EvId, Frame,
        Frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t),
        portMAX_DELAY));
}

namespace RadioInterface {
    void SetSnifferFilter(bool Data, bool Mgmt, bool Ctrl) {
        wifi_promiscuous_filter_t F = { .filter_mask = 0 };
        if (Data) F.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
        if (Mgmt) F.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
        if (Ctrl) F.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
        esp_wifi_set_promiscuous_filter(&F);
    }

    void StartSniffer(uint8_t Channel) {
        ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
        esp_wifi_set_channel(Channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(&OnRawFrame);
    }

    void StopSniffer() { esp_wifi_set_promiscuous(false); }
}
