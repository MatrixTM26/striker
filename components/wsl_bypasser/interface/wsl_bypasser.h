/**
 * @file wsl_bypasser.h
 * @brief C++ compatible WSL bypasser interface
 */
#pragma once

#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size);
void wsl_bypasser_send_deauth_frame(const wifi_ap_record_t *ap_record);

#ifdef __cplusplus
}
#endif
