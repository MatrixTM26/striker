/**
 * @file attack_pmkid.h
 * @brief C++ interface for PMKID attack
 */
#pragma once

#include "attack.h"

#ifdef __cplusplus
extern "C" {
#endif

void attack_pmkid_start(attack_config_t *attack_config);
void attack_pmkid_stop();

#ifdef __cplusplus
}
#endif
