/**
 * @file wifi_controller.cpp
 * @brief WiFi interface management — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "wifi_controller.h"

#include <cstdio>
#include <cstring>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "wifi_controller";

static bool    s_wifi_init = false;
static uint8_t s_original_mac_ap[6];

static void wifi_event_handler(void * /*event_handler_arg*/,
                                esp_event_base_t /*event_base*/,
                                int32_t /*event_id*/,
                                void * /*event_data*/) {
    // event hook — extend here as needed
}

static void wifi_init_apsta() {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, s_original_mac_ap));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_wifi_init = true;
}

extern "C" void wifictl_ap_start(wifi_config_t *wifi_config) {
    ESP_LOGD(TAG, "Starting AP...");
    if (!s_wifi_init) wifi_init_apsta();
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, wifi_config));
    ESP_LOGI(TAG, "AP started — SSID=%s", wifi_config->ap.ssid);
}

extern "C" void wifictl_ap_stop() {
    ESP_LOGD(TAG, "Stopping AP...");
    wifi_config_t cfg = {};
    cfg.ap.max_connection = 0;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &cfg));
}

extern "C" void wifictl_mgmt_ap_start() {
    wifi_config_t cfg = {};
    memcpy(cfg.ap.ssid,     CONFIG_MGMT_AP_SSID,     strlen(CONFIG_MGMT_AP_SSID));
    memcpy(cfg.ap.password, CONFIG_MGMT_AP_PASSWORD,  strlen(CONFIG_MGMT_AP_PASSWORD));
    cfg.ap.ssid_len       = strlen(CONFIG_MGMT_AP_SSID);
    cfg.ap.max_connection = CONFIG_MGMT_AP_MAX_CONNECTIONS;
    cfg.ap.authmode       = WIFI_AUTH_WPA2_PSK;
    wifictl_ap_start(&cfg);
}

extern "C" void wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]) {
    ESP_LOGD(TAG, "Connecting STA to AP...");
    if (!s_wifi_init) wifi_init_apsta();

    wifi_config_t cfg = {};
    cfg.sta.channel          = ap_record->primary;
    cfg.sta.scan_method      = WIFI_FAST_SCAN;
    cfg.sta.pmf_cfg.capable  = false;
    cfg.sta.pmf_cfg.required = false;
    memcpy(cfg.sta.ssid, ap_record->ssid, 32);

    if (password != nullptr) {
        if (strlen(password) >= 64) {
            ESP_LOGE(TAG, "Password too long (max 64 chars)");
            return;
        }
        memcpy(cfg.sta.password, password, strlen(password) + 1);
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

extern "C" void wifictl_sta_disconnect() {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

extern "C" void wifictl_set_ap_mac(const uint8_t *mac_ap) {
    ESP_LOGD(TAG, "Changing AP MAC...");
    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, mac_ap));
}

extern "C" void wifictl_get_ap_mac(uint8_t *mac_ap) {
    esp_wifi_get_mac(WIFI_IF_AP, mac_ap);
}

extern "C" void wifictl_restore_ap_mac() {
    ESP_LOGD(TAG, "Restoring original AP MAC...");
    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, s_original_mac_ap));
}

extern "C" void wifictl_get_sta_mac(uint8_t *mac_sta) {
    esp_wifi_get_mac(WIFI_IF_STA, mac_sta);
}

extern "C" void wifictl_set_channel(uint8_t channel) {
    if (channel == 0 || channel > 13) {
        ESP_LOGE(TAG, "Channel out of range: %u (expected 1–13)", channel);
        return;
    }
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}
