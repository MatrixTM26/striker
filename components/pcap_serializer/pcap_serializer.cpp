/**
 * @file pcap_serializer.cpp
 * @brief PCAP format serializer — converted to C++
 * @author risinek (risinek@gmail.com) | C++ port: Teuku Maulana (TM26)
 */

#include "pcap_serializer.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char *TAG = "pcap_serializer";

constexpr uint32_t SNAPLEN           = 65535;
constexpr uint32_t PCAP_MAGIC_NUMBER = 0xa1b2c3d4;
constexpr uint32_t LINKTYPE_802_11   = 105;

// Use a dynamic buffer backed by std::vector for safe reallocation
static std::vector<uint8_t> s_pcap_buf;

extern "C" uint8_t *pcap_serializer_init() {
    s_pcap_buf.clear();

    const pcap_global_header_t gh = {
        .magic_number  = PCAP_MAGIC_NUMBER,
        .version_major = 2,
        .version_minor = 4,
        .thiszone      = 0,
        .sigfigs       = 0,
        .snaplen       = SNAPLEN,
        .network       = LINKTYPE_802_11
    };

    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&gh);
    s_pcap_buf.insert(s_pcap_buf.end(), ptr, ptr + sizeof(gh));
    return s_pcap_buf.data();
}

extern "C" void pcap_serializer_append_frame(const uint8_t *buffer, unsigned size, unsigned ts_usec) {
    if (size == 0) {
        ESP_LOGD(TAG, "Frame size is 0 — skipping");
        return;
    }

    unsigned incl = (size > SNAPLEN) ? SNAPLEN : size;

    const pcap_record_header_t rh = {
        .ts_sec  = ts_usec / 1000000,
        .ts_usec = ts_usec % 1000000,
        .incl_len = incl,
        .orig_len = size
    };

    const uint8_t *rh_ptr = reinterpret_cast<const uint8_t *>(&rh);
    s_pcap_buf.insert(s_pcap_buf.end(), rh_ptr, rh_ptr + sizeof(rh));
    s_pcap_buf.insert(s_pcap_buf.end(), buffer, buffer + incl);
}

extern "C" void pcap_serializer_deinit() {
    s_pcap_buf.clear();
    s_pcap_buf.shrink_to_fit();
}

extern "C" unsigned pcap_serializer_get_size() {
    return static_cast<unsigned>(s_pcap_buf.size());
}

extern "C" uint8_t *pcap_serializer_get_buffer() {
    return s_pcap_buf.data();
}
