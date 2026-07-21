/**
 * @file frame_analyzer.h
 * @brief C++ interface for frame analysis
 */
#pragma once

#include <cstdint>
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(FRAME_ANALYZER_EVENTS);

enum {
    DATA_FRAME_EVENT_EAPOLKEY_FRAME,
    DATA_FRAME_EVENT_PMKID
};

typedef enum {
    SEARCH_HANDSHAKE,
    SEARCH_PMKID
} search_type_t;

#ifdef __cplusplus
extern "C" {
#endif

void frame_analyzer_capture_start(search_type_t search_type, const uint8_t *bssid);
void frame_analyzer_capture_stop();

#ifdef __cplusplus
}
#endif
