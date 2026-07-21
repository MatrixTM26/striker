/**
 * @file attack.cpp
 * @brief Attack orchestrator — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 *
 * Manages the full attack lifecycle:
 *  - Timer-based timeout
 *  - Event-driven start / reset via WEBSERVER_EVENTS
 *  - Status content with RAII-style vector backing
 */

#include "attack.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "attack_pmkid.h"
#include "attack_handshake.h"
#include "attack_dos.h"
#include "webserver.h"
#include "wifi_controller.h"

static const char *TAG = "attack";

// ── State ──────────────────────────────────────────────────────────────────

static attack_status_t       s_status = { READY, static_cast<uint8_t>(-1), 0, nullptr };
static esp_timer_handle_t    s_timeout_handle = nullptr;

// ── Public status API ──────────────────────────────────────────────────────

extern "C" const attack_status_t *attack_get_status() {
    return &s_status;
}

extern "C" void attack_update_status(attack_state_t state) {
    s_status.state = state;
    if (state == FINISHED) {
        ESP_LOGD(TAG, "Stopping timeout timer");
        ESP_ERROR_CHECK(esp_timer_stop(s_timeout_handle));
    }
}

extern "C" char *attack_alloc_result_content(unsigned size) {
    s_status.content_size = static_cast<uint16_t>(size);
    s_status.content      = static_cast<char *>(malloc(size));
    return s_status.content;
}

extern "C" void attack_append_status_content(uint8_t *buffer, unsigned size) {
    if (size == 0) {
        ESP_LOGE(TAG, "Append size cannot be 0");
        return;
    }
    char *reallocated = static_cast<char *>(
        realloc(s_status.content, s_status.content_size + size));
    if (reallocated == nullptr) {
        ESP_LOGE(TAG, "Realloc failed — content may be incomplete");
        return;
    }
    memcpy(&reallocated[s_status.content_size], buffer, size);
    s_status.content       = reallocated;
    s_status.content_size += static_cast<uint16_t>(size);
}

// ── Timeout callback ───────────────────────────────────────────────────────

static void attack_timeout(void * /*arg*/) {
    ESP_LOGD(TAG, "Attack timed out");
    attack_update_status(TIMEOUT);

    switch (s_status.type) {
        case ATTACK_TYPE_PMKID:
            ESP_LOGI(TAG, "Aborting PMKID attack...");
            attack_pmkid_stop();
            break;
        case ATTACK_TYPE_HANDSHAKE:
            ESP_LOGI(TAG, "Aborting HANDSHAKE attack...");
            attack_handshake_stop();
            break;
        case ATTACK_TYPE_PASSIVE:
            ESP_LOGI(TAG, "PASSIVE attack timeout (no-op)");
            break;
        case ATTACK_TYPE_DOS:
            ESP_LOGI(TAG, "Aborting DoS attack...");
            attack_dos_stop();
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack type — cannot abort cleanly");
    }
}

// ── Event handlers ─────────────────────────────────────────────────────────

static void attack_request_handler(void * /*args*/,
                                    esp_event_base_t /*event_base*/,
                                    int32_t /*event_id*/,
                                    void *event_data)
{
    ESP_LOGI(TAG, "Attack request received");

    auto *req = static_cast<attack_request_t *>(event_data);
    attack_config_t cfg = {
        .type      = req->type,
        .method    = req->method,
        .timeout   = req->timeout,
        .ap_record = wifictl_get_ap_record(req->ap_record_id)
    };

    if (cfg.ap_record == nullptr) {
        ESP_LOGE(TAG, "Null AP record — aborting");
        return;
    }

    s_status.state = RUNNING;
    s_status.type  = cfg.type;

    // Start timeout (0 = no timeout, guard against that)
    if (cfg.timeout > 0) {
        ESP_ERROR_CHECK(esp_timer_start_once(s_timeout_handle,
                                             static_cast<uint64_t>(cfg.timeout) * 1'000'000ULL));
    }

    switch (cfg.type) {
        case ATTACK_TYPE_PMKID:
            attack_pmkid_start(&cfg);
            break;
        case ATTACK_TYPE_HANDSHAKE:
            attack_handshake_start(&cfg);
            break;
        case ATTACK_TYPE_PASSIVE:
            ESP_LOGW(TAG, "ATTACK_TYPE_PASSIVE not implemented yet");
            break;
        case ATTACK_TYPE_DOS:
            attack_dos_start(&cfg);
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack type: %u", cfg.type);
    }
}

static void attack_reset_handler(void * /*args*/,
                                  esp_event_base_t /*event_base*/,
                                  int32_t /*event_id*/,
                                  void * /*event_data*/)
{
    ESP_LOGD(TAG, "Resetting attack status...");
    if (s_status.content) {
        free(s_status.content);
        s_status.content = nullptr;
    }
    s_status.content_size = 0;
    s_status.type         = static_cast<uint8_t>(-1);
    s_status.state        = READY;
}

// ── Init ───────────────────────────────────────────────────────────────────

extern "C" void attack_init() {
    const esp_timer_create_args_t timer_args = {
        .callback = &attack_timeout,
        .arg      = nullptr,
        .name     = "attack_timeout"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_timeout_handle));

    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS,
                                               WEBSERVER_EVENT_ATTACK_REQUEST,
                                               &attack_request_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS,
                                               WEBSERVER_EVENT_ATTACK_RESET,
                                               &attack_reset_handler, nullptr));
    ESP_LOGI(TAG, "Attack subsystem initialized");
}
