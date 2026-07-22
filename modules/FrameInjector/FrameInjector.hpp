#pragma once
#include "esp_wifi_types.h"

namespace FrameInjector {
    void SendRawFrame(const uint8_t* Buffer, int Size);
    void SendDeauthBurst(const wifi_ap_record_t* Target);
}
