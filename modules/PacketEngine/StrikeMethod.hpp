#pragma once
#include "esp_wifi_types.h"

namespace StrikeMethod {
    void LaunchBroadcast(const wifi_ap_record_t* Target, unsigned PeriodSec);
    void HaltBroadcast();
    void LaunchRogueAp(const wifi_ap_record_t* Target);
}
