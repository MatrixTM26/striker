#include "PacketEngine.hpp"
#include "EventBus.hpp"
#include "StrikeMethod.hpp"
#include "StrikePmkid.hpp"
#include "StrikeHandshake.hpp"
#include "StrikeDisrupt.hpp"
#include <cstdlib>
#include <cstring>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "RadioInterface.hpp"

static const char* Tag = "PacketEngine";

static EngineStatus Status = { StateIdle, static_cast<uint8_t>(-1), 0, nullptr };
static esp_timer_handle_t DurationTimer = nullptr;

static void OnDurationExpired(void*) {
    PacketEngine::SetState(StateExpired);
    switch (Status.Mode) {
        case ModePmkid:     StrikePmkid::Stop();    break;
        case ModeHandshake: StrikeHandshake::Stop(); break;
        case ModeDisrupt:   StrikeDisrupt::Stop();   break;
        default: break;
    }
}

static void OnLaunchRequest(void*, esp_event_base_t, int32_t, void* Data) {
    auto* Req = static_cast<StrikeRequest*>(Data);
    EngineConfig Cfg = {
        .Mode     = Req->Mode,
        .Method   = Req->Method,
        .Duration = Req->Duration,
        .Target   = RadioInterface::GetNetwork(Req->TargetId)
    };
    if (!Cfg.Target) return;

    Status.State = StateActive;
    Status.Mode  = Cfg.Mode;

    if (Cfg.Duration > 0)
        ESP_ERROR_CHECK(esp_timer_start_once(DurationTimer,
            static_cast<uint64_t>(Cfg.Duration) * 1'000'000ULL));

    switch (Cfg.Mode) {
        case ModePmkid:     StrikePmkid::Start(&Cfg);    break;
        case ModeHandshake: StrikeHandshake::Start(&Cfg); break;
        case ModeDisrupt:   StrikeDisrupt::Start(&Cfg);   break;
        case ModeSilent: break;
        default: break;
    }
}

static void OnResetRequest(void*, esp_event_base_t, int32_t, void*) {
    if (Status.Payload) { free(Status.Payload); Status.Payload = nullptr; }
    Status.PayloadSize = 0;
    Status.Mode        = static_cast<uint8_t>(-1);
    Status.State       = StateIdle;
}

namespace PacketEngine {
    const EngineStatus* GetStatus() { return &Status; }

    void SetState(EngineState State) {
        Status.State = State;
        if (State == StateDone)
            ESP_ERROR_CHECK(esp_timer_stop(DurationTimer));
    }

    char* AllocPayload(unsigned Size) {
        Status.PayloadSize = static_cast<uint16_t>(Size);
        Status.Payload     = static_cast<char*>(malloc(Size));
        return Status.Payload;
    }

    void AppendPayload(uint8_t* Buffer, unsigned Size) {
        if (!Size) return;
        char* Grown = static_cast<char*>(realloc(Status.Payload, Status.PayloadSize + Size));
        if (!Grown) return;
        memcpy(&Grown[Status.PayloadSize], Buffer, Size);
        Status.Payload      = Grown;
        Status.PayloadSize += static_cast<uint16_t>(Size);
    }

    void Boot() {
        const esp_timer_create_args_t TimerArgs = {
            .callback = &OnDurationExpired,
            .arg      = nullptr,
            .name     = "DurationTimer"
        };
        ESP_ERROR_CHECK(esp_timer_create(&TimerArgs, &DurationTimer));
        ESP_ERROR_CHECK(esp_event_handler_register(
            HttpServerEvents, EventStrikeRequest, &OnLaunchRequest, nullptr));
        ESP_ERROR_CHECK(esp_event_handler_register(
            HttpServerEvents, EventResetRequest, &OnResetRequest, nullptr));
    }
}
