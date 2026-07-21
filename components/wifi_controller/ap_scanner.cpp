/**
 * @file ap_scanner.cpp
 * @brief AP scanning functionality — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "ap_scanner.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"

static const char *TAG = "wifi_controller/ap_scanner";

static wifictl_ap_records_t s_ap_records;

extern "C" void wifictl_scan_nearby_aps() {
    ESP_LOGD(TAG, "Scanning nearby APs...");
    s_ap_records.count = CONFIG_SCAN_MAX_AP;

    wifi_scan_config_t scan_config = {
        .ssid    = nullptr,
        .bssid   = nullptr,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&s_ap_records.count, s_ap_records.records));
    ESP_LOGI(TAG, "Found %u APs.", s_ap_records.count);
}

extern "C" const wifictl_ap_records_t *wifictl_get_ap_records() {
    return &s_ap_records;
}

extern "C" const wifi_ap_record_t *wifictl_get_ap_record(unsigned index) {
    if (index > s_ap_records.count) {
        ESP_LOGE(TAG, "Index out of bounds! %u records available, %u requested",
                 s_ap_records.count, index);
        return nullptr;
    }
    return &s_ap_records.records[index];
}
