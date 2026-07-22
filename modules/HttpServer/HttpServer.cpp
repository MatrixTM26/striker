#include "HttpServer.hpp"
#include <cstring>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "RadioInterface.hpp"
#include "PacketEngine.hpp"
#include "CaptureWriter.hpp"
#include "pages/page_index.h"

static const char* Tag = "HttpServer";

ESP_EVENT_DEFINE_BASE(HttpServerEvents);

static esp_err_t ServeIndex(httpd_req_t* Req) {
    httpd_resp_set_type(Req, "text/html");
    httpd_resp_set_hdr(Req, "Content-Encoding", "gzip");
    return httpd_resp_send(Req,
        reinterpret_cast<const char*>(page_index),
        static_cast<ssize_t>(page_index_len));
}

static esp_err_t ServeReset(httpd_req_t* Req) {
    ESP_ERROR_CHECK(esp_event_post(HttpServerEvents, EventResetRequest,
        nullptr, 0, portMAX_DELAY));
    return httpd_resp_send(Req, nullptr, 0);
}

static esp_err_t ServeNetworkList(httpd_req_t* Req) {
    RadioInterface::ScanNetworks();
    const NetworkList* List = RadioInterface::GetNetworkList();
    ESP_ERROR_CHECK(httpd_resp_set_type(Req, HTTPD_TYPE_OCTET));
    char Chunk[40];
    for (unsigned i = 0; i < List->Count; i++) {
        memcpy(Chunk,      List->Records[i].ssid,  33);
        memcpy(&Chunk[33], List->Records[i].bssid,  6);
        memcpy(&Chunk[39], &List->Records[i].rssi,  1);
        ESP_ERROR_CHECK(httpd_resp_send_chunk(Req, Chunk, 40));
    }
    return httpd_resp_send_chunk(Req, Chunk, 0);
}

static esp_err_t ServeStrikeRequest(httpd_req_t* Req) {
    StrikeRequest Request;
    httpd_req_recv(Req, reinterpret_cast<char*>(&Request), sizeof(StrikeRequest));
    esp_err_t Result = httpd_resp_send(Req, nullptr, 0);
    ESP_ERROR_CHECK(esp_event_post(HttpServerEvents, EventStrikeRequest,
        &Request, sizeof(StrikeRequest), portMAX_DELAY));
    return Result;
}

static esp_err_t ServeStatus(httpd_req_t* Req) {
    const EngineStatus* Status = PacketEngine::GetStatus();
    ESP_ERROR_CHECK(httpd_resp_set_type(Req, HTTPD_TYPE_OCTET));
    ESP_ERROR_CHECK(httpd_resp_send_chunk(Req,
        reinterpret_cast<const char*>(Status), 4));
    if ((Status->State == StateDone || Status->State == StateExpired)
            && Status->PayloadSize > 0)
        ESP_ERROR_CHECK(httpd_resp_send_chunk(Req, Status->Payload, Status->PayloadSize));
    return httpd_resp_send_chunk(Req, nullptr, 0);
}

static esp_err_t ServePcap(httpd_req_t* Req) {
    ESP_ERROR_CHECK(httpd_resp_set_type(Req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(Req,
        reinterpret_cast<const char*>(PcapWriter::GetBuffer()),
        static_cast<ssize_t>(PcapWriter::GetSize()));
}

static esp_err_t ServeHccapx(httpd_req_t* Req) {
    ESP_ERROR_CHECK(httpd_resp_set_type(Req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(Req,
        reinterpret_cast<const char*>(HccapxWriter::Get()),
        sizeof(HccapxRecord));
}

static const httpd_uri_t Routes[] = {
    { "/",               HTTP_GET,  ServeIndex,         nullptr },
    { "/reset",          HTTP_HEAD, ServeReset,         nullptr },
    { "/networks",       HTTP_GET,  ServeNetworkList,   nullptr },
    { "/strike",         HTTP_POST, ServeStrikeRequest, nullptr },
    { "/status",         HTTP_GET,  ServeStatus,        nullptr },
    { "/capture.pcap",   HTTP_GET,  ServePcap,          nullptr },
    { "/capture.hccapx", HTTP_GET,  ServeHccapx,        nullptr },
};

namespace HttpServer {
    void Launch() {
        httpd_config_t Cfg = HTTPD_DEFAULT_CONFIG();
        httpd_handle_t Server = nullptr;
        ESP_ERROR_CHECK(httpd_start(&Server, &Cfg));
        for (const auto& Route : Routes)
            ESP_ERROR_CHECK(httpd_register_uri_handler(Server, &Route));
    }
}
