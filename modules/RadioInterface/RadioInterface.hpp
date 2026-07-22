#pragma once
#include <cstdint>
#include <cstdbool>
#include "esp_event.h"
#include "esp_wifi_types.h"

ESP_EVENT_DECLARE_BASE(RadioScannerEvents);

enum {
    EventPacketData,
    EventPacketManagement,
    EventPacketControl
};

typedef struct {
    uint16_t Count;
    wifi_ap_record_t Records[CONFIG_SCAN_MAX_AP];
} NetworkList;

namespace RadioInterface {
    void     StartManagementAp();
    void     StartAccessPoint(wifi_config_t* Config);
    void     StopAccessPoint();
    void     ConnectStation(const wifi_ap_record_t* Target, const char* Password);
    void     DisconnectStation();
    void     SetApMac(const uint8_t* Mac);
    void     GetApMac(uint8_t* Mac);
    void     RestoreApMac();
    void     GetStaMac(uint8_t* Mac);
    void     SetChannel(uint8_t Channel);
    void     ScanNetworks();
    const NetworkList* GetNetworkList();
    const wifi_ap_record_t* GetNetwork(unsigned Index);
    void     StartSniffer(uint8_t Channel);
    void     StopSniffer();
    void     SetSnifferFilter(bool Data, bool Mgmt, bool Ctrl);
}
