#include "RadioInterface.hpp"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"

static NetworkList Catalog;

namespace RadioInterface {
    void ScanNetworks() {
        Catalog.Count = CONFIG_RADAR_MAX_TARGETS;
        wifi_scan_config_t Cfg = {};
        Cfg.ssid      = nullptr;
        Cfg.bssid     = nullptr;
        Cfg.channel   = 0;
        Cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        ESP_ERROR_CHECK(esp_wifi_scan_start(&Cfg, true));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&Catalog.Count, Catalog.Records));
    }

    const NetworkList* GetNetworkList() { return &Catalog; }

    const wifi_ap_record_t* GetNetwork(unsigned Index) {
        if (Index > Catalog.Count) return nullptr;
        return &Catalog.Records[Index];
    }
}
