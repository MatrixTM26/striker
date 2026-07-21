/**
 * @file pcap_serializer.h
 * @brief C++ compatible PCAP serializer interface
 */
#pragma once

#include <cstdint>

typedef struct {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
} pcap_global_header_t;

typedef struct {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
} pcap_record_header_t;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t *pcap_serializer_init();
void     pcap_serializer_append_frame(const uint8_t *buffer, unsigned size, unsigned ts_usec);
void     pcap_serializer_deinit();
unsigned pcap_serializer_get_size();
uint8_t *pcap_serializer_get_buffer();

#ifdef __cplusplus
}
#endif
