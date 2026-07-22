#pragma once
#include <cstdint>
#include <cstdbool>
#include "esp_wifi_types.h"
#include "FrameTypes.hpp"

namespace FrameDecoder {
    bool          BssidMatches(wifi_promiscuous_pkt_t* Frame, uint8_t* Bssid);
    EapolPacket*  ExtractEapol(DataFrame* Frame);
    EapolKeyPacket* ExtractEapolKey(EapolPacket* Eapol);
    PmkidNode*    ExtractPmkid(EapolKeyPacket* EapolKey);
}
