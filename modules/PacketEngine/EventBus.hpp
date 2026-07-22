#pragma once
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(HttpServerEvents);

enum {
    EventStrikeRequest,
    EventResetRequest
};
