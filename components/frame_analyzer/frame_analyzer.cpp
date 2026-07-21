/**
 * @file frame_analyzer.cpp
 * @brief Frame analysis logic — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "frame_analyzer.h"

#include <cstdint>
#include <cstring>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "wifi_controller.h"
#include "frame_analyzer_parser.h"

static const char *TAG = "frame_analyzer";

// Module-level state (namespace-scoped equivalent using static)
static uint8_t      s_target_bssid[6];
static search_type_t s_search_type = static_cast<search_type_t>(-1);

// ── Internal frame handler ─────────────────────────────────────────────────

static void data_frame_handler(void * /*args*/,
                                esp_event_base_t /*event_base*/,
                                int32_t /*event_id*/,
                                void *event_data)
{
    ESP_LOGV(TAG, "Handling DATA frame");
    auto *frame = static_cast<wifi_promiscuous_pkt_t *>(event_data);

    if (!is_frame_bssid_matching(frame, s_target_bssid)) {
        ESP_LOGV(TAG, "BSSID mismatch — skipping");
        return;
    }

    eapol_packet_t *eapol = parse_eapol_packet(reinterpret_cast<data_frame_t *>(frame->payload));
    if (eapol == nullptr) {
        ESP_LOGV(TAG, "Not an EAPOL packet");
        return;
    }

    eapol_key_packet_t *eapol_key = parse_eapol_key_packet(eapol);
    if (eapol_key == nullptr) {
        ESP_LOGV(TAG, "Not an EAPOL-Key packet");
        return;
    }

    if (s_search_type == SEARCH_HANDSHAKE) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME,
                           frame,
                           sizeof(wifi_promiscuous_pkt_t) + frame->rx_ctrl.sig_len,
                           portMAX_DELAY));
        return;
    }

    if (s_search_type == SEARCH_PMKID) {
        pmkid_item_t *pmkid_items = parse_pmkid(eapol_key);
        if (pmkid_items == nullptr) return;
        ESP_ERROR_CHECK(
            esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID,
                           &pmkid_items, sizeof(pmkid_item_t *), portMAX_DELAY));
    }
}

// ── Public API ─────────────────────────────────────────────────────────────

extern "C" void frame_analyzer_capture_start(search_type_t search_type_arg, const uint8_t *bssid) {
    ESP_LOGI(TAG, "Frame analysis started...");
    s_search_type = search_type_arg;
    memcpy(s_target_bssid, bssid, 6);
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA,
                                               &data_frame_handler, nullptr));
}

extern "C" void frame_analyzer_capture_stop() {
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID,
                                                 &data_frame_handler));
}
