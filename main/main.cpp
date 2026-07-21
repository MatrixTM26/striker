/**
 * @file main.cpp
 * @brief Application entry point — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 *
 * Boots the ESP32 into management AP mode, initialises the attack subsystem
 * and starts the HTTP webserver.
 */

#include <cstdio>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_event.h"

#include "attack.h"
#include "wifi_controller.h"
#include "webserver.h"

static const char *TAG = "main";

extern "C" void app_main(void) {
    ESP_LOGD(TAG, "app_main() — ESP32 WiFi PenTest Tool starting");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifictl_mgmt_ap_start();
    attack_init();
    webserver_run();
    ESP_LOGI(TAG, "System ready");
}
