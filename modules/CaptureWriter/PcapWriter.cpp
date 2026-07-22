#include "CaptureWriter.hpp"
#include <cstring>
#include <vector>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char* Tag = "PcapWriter";

constexpr uint32_t SnapLen       = 65535;
constexpr uint32_t PcapMagic     = 0xa1b2c3d4;
constexpr uint32_t LinkTypeWifi  = 105;

static std::vector<uint8_t> PcapBuffer;

namespace PcapWriter {
    uint8_t* Init() {
        PcapBuffer.clear();
        const PcapFileHeader Hdr = {
            .MagicNumber       = PcapMagic,
            .VersionMajor      = 2,
            .VersionMinor      = 4,
            .TimeZone          = 0,
            .TimestampAccuracy = 0,
            .SnapshotLength    = SnapLen,
            .LinkType          = LinkTypeWifi
        };
        const uint8_t* Ptr = reinterpret_cast<const uint8_t*>(&Hdr);
        PcapBuffer.insert(PcapBuffer.end(), Ptr, Ptr + sizeof(Hdr));
        return PcapBuffer.data();
    }

    void AppendFrame(const uint8_t* Buffer, unsigned Size, unsigned TimestampUs) {
        if (Size == 0) return;
        unsigned Captured = (Size > SnapLen) ? SnapLen : Size;
        const PcapRecordHeader Rec = {
            .TimestampSec  = TimestampUs / 1000000,
            .TimestampUsec = TimestampUs % 1000000,
            .CapturedLength = Captured,
            .OriginalLength = Size
        };
        const uint8_t* RecPtr = reinterpret_cast<const uint8_t*>(&Rec);
        PcapBuffer.insert(PcapBuffer.end(), RecPtr, RecPtr + sizeof(Rec));
        PcapBuffer.insert(PcapBuffer.end(), Buffer, Buffer + Captured);
    }

    void     Flush()     { PcapBuffer.clear(); PcapBuffer.shrink_to_fit(); }
    unsigned GetSize()   { return static_cast<unsigned>(PcapBuffer.size()); }
    uint8_t* GetBuffer() { return PcapBuffer.data(); }
}
