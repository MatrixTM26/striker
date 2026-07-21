/**
 * @file frame_analyzer_parser.h
 * @brief C++ interface for 802.11 / EAPoL frame parsing
 */
#pragma once

#include <cstdint>
#include <cstdbool>
#include "esp_wifi_types.h"
#include "frame_analyzer_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool            is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid);
eapol_packet_t *parse_eapol_packet(data_frame_t *frame);
eapol_key_packet_t *parse_eapol_key_packet(eapol_packet_t *eapol_packet);
pmkid_item_t   *parse_pmkid(eapol_key_packet_t *eapol_key);

#ifdef __cplusplus
}
#endif
