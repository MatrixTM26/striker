/**
 * @file frame_analyzer_parser.cpp
 * @brief 802.11 / EAPoL frame parsing — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "frame_analyzer_parser.h"

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "arpa/inet.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "frame_analyzer_types.h"

static const char *TAG = "frame_analyzer:parser";

ESP_EVENT_DEFINE_BASE(FRAME_ANALYZER_EVENTS);

// ── Debug helpers ──────────────────────────────────────────────────────────

static void print_raw_frame(const wifi_promiscuous_pkt_t *frame) {
    for (unsigned i = 0; i < frame->rx_ctrl.sig_len; i++) {
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
}

static void print_mac_address(const uint8_t *a) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
           a[0], a[1], a[2], a[3], a[4], a[5]);
}

// ── Public API ─────────────────────────────────────────────────────────────

extern "C" bool is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid) {
    const auto *mac_header = reinterpret_cast<data_frame_mac_header_t *>(frame->payload);
    return memcmp(mac_header->addr3, bssid, 6) == 0;
}

extern "C" eapol_packet_t *parse_eapol_packet(data_frame_t *frame) {
    uint8_t *buf = frame->body;

    if (frame->mac_header.frame_control.protected_frame == 1) {
        ESP_LOGV(TAG, "Protected frame, skipping...");
        return nullptr;
    }

    if (frame->mac_header.frame_control.subtype > 7) {
        ESP_LOGV(TAG, "QoS data frame — skipping QoS field");
        buf += 2;   // QoS field
    }

    buf += sizeof(llc_snap_header_t);  // Skip LLC SNAP header

    if (ntohs(*reinterpret_cast<uint16_t *>(buf)) == ETHER_TYPE_EAPOL) {
        ESP_LOGD(TAG, "EAPOL packet found");
        buf += 2;
        return reinterpret_cast<eapol_packet_t *>(buf);
    }
    return nullptr;
}

extern "C" eapol_key_packet_t *parse_eapol_key_packet(eapol_packet_t *eapol_packet) {
    if (eapol_packet->header.packet_type != EAPOL_KEY) {
        ESP_LOGD(TAG, "Not an EAPoL-Key packet.");
        return nullptr;
    }
    return reinterpret_cast<eapol_key_packet_t *>(eapol_packet->packet_body);
}

// ── PMKID parsing ─────────────────────────────────────────────────────────

static pmkid_item_t *parse_pmkid_from_key_data(uint8_t *key_data, uint16_t length) {
    uint8_t *idx     = key_data;
    uint8_t *idx_max = key_data + length;

    pmkid_item_t *head = nullptr;

    do {
        auto *field = reinterpret_cast<key_data_field_t *>(idx);

        ESP_LOGV(TAG, "Key-Data type=0x%x length=0x%x oui=0x%x data_type=0x%x",
                 field->type, field->length, field->oui, field->data_type);

        if (field->type != KEY_DATA_TYPE) {
            ESP_LOGD(TAG, "Wrong type 0x%x (expected 0x%x)", field->type, KEY_DATA_TYPE);
            goto next;
        }
        if (ntohl(field->oui) != KEY_DATA_OUI_IEEE80211) {
            ESP_LOGD(TAG, "Wrong OUI 0x%x", field->oui);
            goto next;
        }
        if (field->data_type != KEY_DATA_DATA_TYPE_PMKID_KDE) {
            ESP_LOGD(TAG, "Wrong data_type 0x%x", field->data_type);
            goto next;
        }

        {
            ESP_LOGI(TAG, "Found PMKID:");
            auto *item = static_cast<pmkid_item_t *>(malloc(sizeof(pmkid_item_t)));
            item->next = head;
            head = item;
            for (unsigned i = 0; i < 16; i++) {
                item->pmkid[i] = field->data[i];
                printf("%02x", item->pmkid[i]);
            }
            printf("\n");
        }

        next:
        idx = field->data + field->length - 4 + 1;

    } while (idx < idx_max);

    return head;
}

extern "C" pmkid_item_t *parse_pmkid(eapol_key_packet_t *eapol_key) {
    if (eapol_key->key_data_length == 0) {
        ESP_LOGD(TAG, "Empty Key Data");
        return nullptr;
    }
    if (eapol_key->key_information.encrypted_key_data == 1) {
        ESP_LOGD(TAG, "Key Data encrypted");
        return nullptr;
    }
    return parse_pmkid_from_key_data(eapol_key->key_data, ntohs(eapol_key->key_data_length));
}
