#pragma once
#include <cstdint>
#include "esp_wifi_types.h"

typedef enum : uint8_t {
    ModeSilent    = 0,
    ModeHandshake = 1,
    ModePmkid     = 2,
    ModeDisrupt   = 3
} OperationMode;

typedef enum : uint8_t {
    StateIdle    = 0,
    StateActive  = 1,
    StateDone    = 2,
    StateExpired = 3
} EngineState;

typedef struct {
    uint8_t  TargetId;
    uint8_t  Mode;
    uint8_t  Method;
    uint8_t  Duration;
} StrikeRequest;

typedef struct {
    uint8_t  Mode;
    uint8_t  Method;
    uint8_t  Duration;
    const wifi_ap_record_t* Target;
} EngineConfig;

typedef struct {
    uint8_t  State;
    uint8_t  Mode;
    uint16_t PayloadSize;
    char*    Payload;
} EngineStatus;

namespace PacketEngine {
    const EngineStatus* GetStatus();
    void SetState(EngineState State);
    void Boot();
    char* AllocPayload(unsigned Size);
    void  AppendPayload(uint8_t* Buffer, unsigned Size);
}
