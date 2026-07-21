/**
 * @file sniffer.cpp
 * @brief Promiscuous mode sniffer — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "sniffer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "sniffer";

ESP_EVENT_DEFINE_BASE(SNIFFER_EVENTS);

static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ESP_LOGV(TAG, "Captured frame type=%d", static_cast<int>(type));

    auto *frame = static_cast<wifi_promiscuous_pkt_t *>(buf);
    int32_t event_id;

    switch (type) {
        case WIFI_PKT_DATA: event_id = SNIFFER_EVENT_CAPTURED_DATA; break;
        case WIFI_PKT_MGMT: event_id = SNIFFER_EVENT_CAPTURED_MGMT; break;
        case WIFI_PKT_CTRL: event_id = SNIFFER_EVENT_CAPTURED_CTRL; break;
        default: return;
    }

    ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, event_id,
                                   frame,
                                   frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t),
                                   portMAX_DELAY));
}

extern "C" void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl) {
    wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
    if (data) filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    if (mgmt) filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    if (ctrl) filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    esp_wifi_set_promiscuous_filter(&filter);
}

extern "C" void wifictl_sniffer_start(uint8_t channel) {
    ESP_LOGI(TAG, "Starting promiscuous mode on channel %u...", channel);
    ESP_LOGD(TAG, "Kicking all connected STAs");
    ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
}

extern "C" void wifictl_sniffer_stop() {
    ESP_LOGI(TAG, "Stopping promiscuous mode...");
    esp_wifi_set_promiscuous(false);
}
