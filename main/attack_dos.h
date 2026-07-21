/**
 * @file attack_dos.h
 * @brief C++ interface for DoS attack
 */
#pragma once

#include "attack.h"

typedef enum {
    ATTACK_DOS_METHOD_ROGUE_AP,
    ATTACK_DOS_METHOD_BROADCAST,
    ATTACK_DOS_METHOD_COMBINE_ALL
} attack_dos_methods_t;

#ifdef __cplusplus
extern "C" {
#endif

void attack_dos_start(attack_config_t *attack_config);
void attack_dos_stop();

#ifdef __cplusplus
}
#endif
