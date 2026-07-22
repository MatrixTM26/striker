#include "RadioInterface.hpp"
#include <cstring>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char* Tag = "RadioCore";
static bool Initialized = false;
static uint8_t OriginalApMac[6];

static void OnWifiEvent(void*, esp_event_base_t, int32_t, void*) {}

static void InitApSta() {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t Cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&Cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &OnWifiEvent, nullptr));
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, OriginalApMac));
    ESP_ERROR_CHECK(esp_wifi_start());
    Initialized = true;
}

namespace RadioInterface {
    void StartAccessPoint(wifi_config_t* Config) {
        if (!Initialized) InitApSta();
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, Config));
    }

    void StopAccessPoint() {
        wifi_config_t Cfg = {};
        Cfg.ap.max_connection = 0;
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &Cfg));
    }

    void StartManagementAp() {
        wifi_config_t Cfg = {};
        memcpy(Cfg.ap.ssid,     CONFIG_MGMT_AP_SSID,     strlen(CONFIG_MGMT_AP_SSID));
        memcpy(Cfg.ap.password, CONFIG_MGMT_AP_PASSWORD,  strlen(CONFIG_MGMT_AP_PASSWORD));
        Cfg.ap.ssid_len       = strlen(CONFIG_MGMT_AP_SSID);
        Cfg.ap.max_connection = CONFIG_MGMT_AP_MAX_CONNECTIONS;
        Cfg.ap.authmode       = WIFI_AUTH_WPA2_PSK;
        StartAccessPoint(&Cfg);
    }

    void ConnectStation(const wifi_ap_record_t* Target, const char* Password) {
        if (!Initialized) InitApSta();
        wifi_config_t Cfg = {};
        Cfg.sta.channel          = Target->primary;
        Cfg.sta.scan_method      = WIFI_FAST_SCAN;
        Cfg.sta.pmf_cfg.capable  = false;
        Cfg.sta.pmf_cfg.required = false;
        memcpy(Cfg.sta.ssid, Target->ssid, 32);
        if (Password && strlen(Password) < 64)
            memcpy(Cfg.sta.password, Password, strlen(Password) + 1);
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &Cfg));
        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    void DisconnectStation() { ESP_ERROR_CHECK(esp_wifi_disconnect()); }

    void SetApMac(const uint8_t* Mac) { ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, Mac)); }
    void GetApMac(uint8_t* Mac)       { esp_wifi_get_mac(WIFI_IF_AP, Mac); }
    void RestoreApMac()               { ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, OriginalApMac)); }
    void GetStaMac(uint8_t* Mac)      { esp_wifi_get_mac(WIFI_IF_STA, Mac); }

    void SetChannel(uint8_t Channel) {
        if (Channel == 0 || Channel > 13) return;
        esp_wifi_set_channel(Channel, WIFI_SECOND_CHAN_NONE);
    }
}
