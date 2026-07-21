/**
 * @file ap_scanner.h
 * @brief C++ compatible AP scanner interface
 */
#pragma once

#include <cstdint>
#include "esp_wifi_types.h"

typedef struct {
    uint16_t count;
    wifi_ap_record_t records[CONFIG_SCAN_MAX_AP];
} wifictl_ap_records_t;

#ifdef __cplusplus
extern "C" {
#endif

void                    wifictl_scan_nearby_aps();
const wifictl_ap_records_t *wifictl_get_ap_records();
const wifi_ap_record_t     *wifictl_get_ap_record(unsigned index);

#ifdef __cplusplus
}
#endif
