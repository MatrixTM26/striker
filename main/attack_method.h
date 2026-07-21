/**
 * @file attack_method.h
 * @brief C++ interface for common attack methods
 */
#pragma once

#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void attack_method_broadcast(const wifi_ap_record_t *ap_record, unsigned period_sec);
void attack_method_broadcast_stop();
void attack_method_rogueap(const wifi_ap_record_t *ap_record);

#ifdef __cplusplus
}
#endif
