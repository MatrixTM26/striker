#pragma once
#include <cstdint>
#include <cstdbool>
#include "esp_event.h"
#include "esp_wifi_types.h"
#include "FrameTypes.hpp"

ESP_EVENT_DECLARE_BASE(FrameParserEvents);

enum {
    EventEapolKeyFrame,
    EventPmkidCaptured
};

typedef enum {
    SearchHandshake,
    SearchPmkid
} SearchMode;

namespace FrameParser {
    void StartCapture(SearchMode Mode, const uint8_t* Bssid);
    void StopCapture();
}
