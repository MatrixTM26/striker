/**
 * @file webserver.cpp
 * @brief HTTP server + REST endpoints — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "webserver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_wifi_types.h"

#include "wifi_controller.h"
#include "attack.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"

#include "pages/page_index.h"

static const char *TAG = "webserver";

ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

// ── GET / ─────────────────────────────────────────────────────────────────

static esp_err_t uri_root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, reinterpret_cast<const char *>(page_index),
                           static_cast<ssize_t>(page_index_len));
}

static const httpd_uri_t uri_root_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = uri_root_get_handler,
    .user_ctx = nullptr
};

// ── HEAD /reset ───────────────────────────────────────────────────────────

static esp_err_t uri_reset_head_handler(httpd_req_t *req) {
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET,
                                   nullptr, 0, portMAX_DELAY));
    return httpd_resp_send(req, nullptr, 0);
}

static const httpd_uri_t uri_reset_head = {
    .uri      = "/reset",
    .method   = HTTP_HEAD,
    .handler  = uri_reset_head_handler,
    .user_ctx = nullptr
};

// ── GET /ap-list ──────────────────────────────────────────────────────────

static esp_err_t uri_ap_list_get_handler(httpd_req_t *req) {
    wifictl_scan_nearby_aps();
    const wifictl_ap_records_t *records = wifictl_get_ap_records();

    char chunk[40];
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));

    for (unsigned i = 0; i < records->count; i++) {
        memcpy(chunk,      records->records[i].ssid,  33);
        memcpy(&chunk[33], records->records[i].bssid,  6);
        memcpy(&chunk[39], &records->records[i].rssi,  1);
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, chunk, 40));
    }
    return httpd_resp_send_chunk(req, chunk, 0);
}

static const httpd_uri_t uri_ap_list_get = {
    .uri      = "/ap-list",
    .method   = HTTP_GET,
    .handler  = uri_ap_list_get_handler,
    .user_ctx = nullptr
};

// ── POST /run-attack ──────────────────────────────────────────────────────

static esp_err_t uri_run_attack_post_handler(httpd_req_t *req) {
    attack_request_t request;
    httpd_req_recv(req, reinterpret_cast<char *>(&request), sizeof(attack_request_t));
    esp_err_t res = httpd_resp_send(req, nullptr, 0);
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST,
                                   &request, sizeof(attack_request_t), portMAX_DELAY));
    return res;
}

static const httpd_uri_t uri_run_attack_post = {
    .uri      = "/run-attack",
    .method   = HTTP_POST,
    .handler  = uri_run_attack_post_handler,
    .user_ctx = nullptr
};

// ── GET /status ───────────────────────────────────────────────────────────

static esp_err_t uri_status_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Fetching attack status...");
    const attack_status_t *status = attack_get_status();

    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, reinterpret_cast<const char *>(status), 4));

    if ((status->state == FINISHED || status->state == TIMEOUT) && status->content_size > 0) {
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, status->content, status->content_size));
    }
    return httpd_resp_send_chunk(req, nullptr, 0);
}

static const httpd_uri_t uri_status_get = {
    .uri      = "/status",
    .method   = HTTP_GET,
    .handler  = uri_status_get_handler,
    .user_ctx = nullptr
};

// ── GET /capture.pcap ────────────────────────────────────────────────────

static esp_err_t uri_capture_pcap_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Serving PCAP file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req,
                           reinterpret_cast<const char *>(pcap_serializer_get_buffer()),
                           static_cast<ssize_t>(pcap_serializer_get_size()));
}

static const httpd_uri_t uri_capture_pcap_get = {
    .uri      = "/capture.pcap",
    .method   = HTTP_GET,
    .handler  = uri_capture_pcap_get_handler,
    .user_ctx = nullptr
};

// ── GET /capture.hccapx ──────────────────────────────────────────────────

static esp_err_t uri_capture_hccapx_get_handler(httpd_req_t *req) {
    ESP_LOGD(TAG, "Serving HCCAPX file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req,
                           reinterpret_cast<const char *>(hccapx_serializer_get()),
                           sizeof(hccapx_t));
}

static const httpd_uri_t uri_capture_hccapx_get = {
    .uri      = "/capture.hccapx",
    .method   = HTTP_GET,
    .handler  = uri_capture_hccapx_get_handler,
    .user_ctx = nullptr
};

// ── Startup ───────────────────────────────────────────────────────────────

extern "C" void webserver_run() {
    ESP_LOGD(TAG, "Starting webserver...");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = nullptr;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_root_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_reset_head));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_ap_list_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_run_attack_post));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_status_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_pcap_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_hccapx_get));
    ESP_LOGI(TAG, "Webserver running");
}
