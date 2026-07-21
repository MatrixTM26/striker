/**
 * @file sniffer.h
 * @brief C++ compatible sniffer interface
 */
#pragma once

#include <cstdbool>
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENTS);

enum {
    SNIFFER_EVENT_CAPTURED_DATA,
    SNIFFER_EVENT_CAPTURED_MGMT,
    SNIFFER_EVENT_CAPTURED_CTRL
};

#ifdef __cplusplus
extern "C" {
#endif

void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl);
void wifictl_sniffer_start(uint8_t channel);
void wifictl_sniffer_stop();

#ifdef __cplusplus
}
#endif
