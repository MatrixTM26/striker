#include "RadioInterface.hpp"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"

static const char* Tag = "RadioScanner";
static NetworkList Catalog;

namespace RadioInterface {
    void ScanNetworks() {
        Catalog.Count = CONFIG_RADAR_MAX_TARGETS;
        wifi_scan_config_t Cfg = {
            .ssid = nullptr, .bssid = nullptr,
            .channel = 0, .scan_type = WIFI_SCAN_TYPE_ACTIVE
        };
        ESP_ERROR_CHECK(esp_wifi_scan_start(&Cfg, true));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&Catalog.Count, Catalog.Records));
    }

    const NetworkList* GetNetworkList() { return &Catalog; }

    const wifi_ap_record_t* GetNetwork(unsigned Index) {
        if (Index > Catalog.Count) return nullptr;
        return &Catalog.Records[Index];
    }
}
