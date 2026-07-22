#pragma once
#include <cstdint>
#include "FrameParser/FrameTypes.hpp"

typedef struct {
    uint32_t MagicNumber;
    uint16_t VersionMajor;
    uint16_t VersionMinor;
    int32_t  TimeZone;
    uint32_t TimestampAccuracy;
    uint32_t SnapshotLength;
    uint32_t LinkType;
} PcapFileHeader;

typedef struct {
    uint32_t TimestampSec;
    uint32_t TimestampUsec;
    uint32_t CapturedLength;
    uint32_t OriginalLength;
} PcapRecordHeader;

typedef struct __attribute__((__packed__)) {
    uint32_t Signature;
    uint32_t Version;
    uint8_t  MessagePair;
    uint8_t  SsidLength;
    uint8_t  Ssid[32];
    uint8_t  KeyVersion;
    uint8_t  KeyMic[16];
    uint8_t  MacAccessPoint[6];
    uint8_t  NonceAp[32];
    uint8_t  MacStation[6];
    uint8_t  NonceSta[32];
    uint16_t EapolLength;
    uint8_t  EapolData[256];
} HccapxRecord;

namespace PcapWriter {
    uint8_t* Init();
    void     AppendFrame(const uint8_t* Buffer, unsigned Size, unsigned TimestampUs);
    void     Flush();
    unsigned GetSize();
    uint8_t* GetBuffer();
}

namespace HccapxWriter {
    void      Init(const uint8_t* Ssid, unsigned Size);
    HccapxRecord* Get();
    void      AddFrame(DataFrame* Frame);
}
