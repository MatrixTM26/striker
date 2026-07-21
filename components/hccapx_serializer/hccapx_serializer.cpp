/**
 * @file hccapx_serializer.cpp
 * @brief HCCAPX format serializer — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "hccapx_serializer.h"

#include <cstdint>
#include <cstring>
#include <algorithm>
#include "arpa/inet.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#include "frame_analyzer.h"
#include "frame_analyzer_types.h"
#include "frame_analyzer_parser.h"

static const char *TAG = "hccapx_serializer";

constexpr uint32_t HCCAPX_SIGNATURE      = 0x58504348;
constexpr uint32_t HCCAPX_VERSION        = 4;
constexpr uint8_t  HCCAPX_KEYVER_WPA     = 1;
constexpr uint8_t  HCCAPX_KEYVER_WPA2    = 2;
constexpr unsigned HCCAPX_MAX_EAPOL_SIZE = 256;

static hccapx_t  s_hccapx = {
    .signature    = HCCAPX_SIGNATURE,
    .version      = HCCAPX_VERSION,
    .message_pair = 255,
    .keyver       = HCCAPX_KEYVER_WPA2
};

static unsigned s_message_ap  = 0;
static unsigned s_message_sta = 0;
static unsigned s_eapol_src   = 0;

// ── Helpers ────────────────────────────────────────────────────────────────

static bool is_array_zero(const uint8_t *arr, unsigned size) {
    for (unsigned i = 0; i < size; i++) {
        if (arr[i] != 0) return false;
    }
    return true;
}

static unsigned save_eapol(eapol_packet_t *eapol, eapol_key_packet_t *eapol_key) {
    unsigned len = sizeof(eapol_packet_header_t) + ntohs(eapol->header.packet_body_length);
    if (len > HCCAPX_MAX_EAPOL_SIZE) {
        ESP_LOGW(TAG, "EAPoL too long (%u/%u)", len, HCCAPX_MAX_EAPOL_SIZE);
        return 1;
    }
    s_hccapx.eapol_len = static_cast<uint16_t>(len);
    memcpy(s_hccapx.eapol, eapol, len);
    memcpy(s_hccapx.keymic, eapol_key->key_mic, 16);
    // Clear MIC from EAPoL so hashcat can recalculate (802.11i-2004 [8.5.2/h])
    memset(&s_hccapx.eapol[81], 0x00, 16);
    return 0;
}

// ── AP message handlers ────────────────────────────────────────────────────

static void ap_message_m1(eapol_key_packet_t *key) {
    ESP_LOGD(TAG, "AP M1");
    s_message_ap = 1;
    memcpy(s_hccapx.nonce_ap, key->key_nonce, 32);
}

static void ap_message_m3(eapol_packet_t *eapol, eapol_key_packet_t *key) {
    ESP_LOGD(TAG, "AP M3");
    s_message_ap = 3;
    if (s_message_ap == 0) memcpy(s_hccapx.nonce_ap, key->key_nonce, 32);
    if (s_eapol_src == 2) { s_hccapx.message_pair = 2; return; }
    if (save_eapol(eapol, key) != 0) return;
    s_eapol_src = 3;
    if (s_message_sta == 2) s_hccapx.message_pair = 3;
}

static void ap_message(data_frame_t *frame, eapol_packet_t *eapol, eapol_key_packet_t *key) {
    if (!is_array_zero(s_hccapx.mac_sta, 6) &&
        memcmp(frame->mac_header.addr1, s_hccapx.mac_sta, 6) != 0) {
        ESP_LOGE(TAG, "Different STA — skipping");
        return;
    }
    if (s_message_ap == 0) memcpy(s_hccapx.mac_ap, frame->mac_header.addr2, 6);
    if (is_array_zero(key->key_mic, 16)) ap_message_m1(key);
    else                                  ap_message_m3(eapol, key);
}

// ── STA message handlers ───────────────────────────────────────────────────

static void sta_message_m2(eapol_packet_t *eapol, eapol_key_packet_t *key) {
    ESP_LOGD(TAG, "STA M2");
    s_message_sta = 2;
    memcpy(s_hccapx.nonce_sta, key->key_nonce, 32);
    if (save_eapol(eapol, key) != 0) return;
    s_eapol_src = 2;
    if (s_message_ap == 1) { s_hccapx.message_pair = 0; return; }
}

static void sta_message_m4(eapol_packet_t *eapol, eapol_key_packet_t *key) {
    ESP_LOGD(TAG, "STA M4");
    if (s_message_sta == 2 && s_eapol_src != 0) {
        ESP_LOGD(TAG, "Already have M2, skipping M4"); return;
    }
    if (s_message_ap == 0) { ESP_LOGE(TAG, "No AP message yet"); return; }
    if (s_eapol_src == 3) { s_hccapx.message_pair = 4; return; }
    if (save_eapol(eapol, key) != 0) return;
    s_eapol_src = 4;
    if (s_message_ap == 1) s_hccapx.message_pair = 1;
    if (s_message_ap == 3) s_hccapx.message_pair = 5;
}

static void sta_message(data_frame_t *frame, eapol_packet_t *eapol, eapol_key_packet_t *key) {
    if (is_array_zero(s_hccapx.mac_sta, 6)) {
        memcpy(s_hccapx.mac_sta, frame->mac_header.addr2, 6);
    } else if (memcmp(frame->mac_header.addr2, s_hccapx.mac_sta, 6) != 0) {
        ESP_LOGE(TAG, "Different STA — skipping"); return;
    }
    if (!is_array_zero(key->key_nonce, 16)) sta_message_m2(eapol, key);
    else                                     sta_message_m4(eapol, key);
}

// ── Public API ─────────────────────────────────────────────────────────────

extern "C" void hccapx_serializer_init(const uint8_t *ssid, unsigned size) {
    s_hccapx.essid_len = static_cast<uint8_t>(size);
    memcpy(s_hccapx.essid, ssid, size);
    s_hccapx.message_pair = 255;
    s_message_ap  = 0;
    s_message_sta = 0;
    s_eapol_src   = 0;
}

extern "C" hccapx_t *hccapx_serializer_get() {
    if (s_hccapx.message_pair == 255) return nullptr;
    return &s_hccapx;
}

extern "C" void hccapx_serializer_add_frame(data_frame_t *frame) {
    eapol_packet_t     *eapol = parse_eapol_packet(frame);
    if (!eapol) return;
    eapol_key_packet_t *key   = parse_eapol_key_packet(eapol);
    if (!key) return;

    // Direction: addr3 is BSSID; addr2 == BSSID → from AP, addr1 == BSSID → from STA
    if (memcmp(frame->mac_header.addr2, frame->mac_header.addr3, 6) == 0) {
        ap_message(frame, eapol, key);
    } else if (memcmp(frame->mac_header.addr1, frame->mac_header.addr3, 6) == 0) {
        sta_message(frame, eapol, key);
    } else {
        ESP_LOGE(TAG, "Unknown frame direction");
    }
}
