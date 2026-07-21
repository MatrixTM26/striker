/**
 * @file attack.h
 * @brief Attack wrapper interface — C++ version
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */
#pragma once

#include <cstdint>
#include "esp_wifi_types.h"

typedef enum : uint8_t {
    ATTACK_TYPE_PASSIVE   = 0,
    ATTACK_TYPE_HANDSHAKE = 1,
    ATTACK_TYPE_PMKID     = 2,
    ATTACK_TYPE_DOS       = 3
} attack_type_t;

typedef enum : uint8_t {
    READY    = 0,
    RUNNING  = 1,
    FINISHED = 2,
    TIMEOUT  = 3
} attack_state_t;

typedef struct {
    uint8_t type;
    uint8_t method;
    uint8_t timeout;
    const wifi_ap_record_t *ap_record;
} attack_config_t;

typedef struct {
    uint8_t  state;
    uint8_t  type;
    uint16_t content_size;
    char    *content;
} attack_status_t;

#ifdef __cplusplus
extern "C" {
#endif

const attack_status_t *attack_get_status();
void   attack_update_status(attack_state_t state);
void   attack_init();
char  *attack_alloc_result_content(unsigned size);
void   attack_append_status_content(uint8_t *buffer, unsigned size);

#ifdef __cplusplus
}
#endif
