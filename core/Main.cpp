#include <cstdio>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_event.h"
#include "RadioInterface.hpp"
#include "PacketEngine.hpp"
#include "HttpServer.hpp"

static const char* Tag = "NetStrike32";

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    RadioInterface::StartManagementAp();
    PacketEngine::Boot();
    HttpServer::Launch();
    ESP_LOGI(Tag, "NetStrike32 online");
}
