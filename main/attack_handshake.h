/**
 * @file attack_handshake.h
 * @brief C++ interface for WPA handshake capture attack
 */
#pragma once

#include "attack.h"

typedef enum {
    ATTACK_HANDSHAKE_METHOD_ROGUE_AP,
    ATTACK_HANDSHAKE_METHOD_BROADCAST,
    ATTACK_HANDSHAKE_METHOD_PASSIVE
} attack_handshake_methods_t;

#ifdef __cplusplus
extern "C" {
#endif

void attack_handshake_start(attack_config_t *attack_config);
void attack_handshake_stop();

#ifdef __cplusplus
}
#endif
