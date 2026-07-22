#include "CaptureWriter.hpp"
#include "FrameParser/FrameTypes.hpp"
#include "FrameParser/FrameDecoder.hpp"
#include <cstring>
#include "arpa/inet.h"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

static const char* Tag = "HccapxWriter";

constexpr uint32_t HccapxSignature  = 0x58504348;
constexpr uint32_t HccapxVersion    = 4;
constexpr uint8_t  KeyVersionWpa2   = 2;
constexpr unsigned MaxEapolSize     = 256;

static HccapxRecord Record = {
    .Signature   = HccapxSignature,
    .Version     = HccapxVersion,
    .MessagePair = 255,
    .KeyVersion  = KeyVersionWpa2
};

static unsigned MsgAp      = 0;
static unsigned MsgSta     = 0;
static unsigned EapolSource = 0;

static bool IsZero(const uint8_t* Arr, unsigned Len) {
    for (unsigned i = 0; i < Len; i++) if (Arr[i]) return false;
    return true;
}

static unsigned SaveEapol(EapolPacket* Eapol, EapolKeyPacket* Key) {
    unsigned Len = sizeof(EapolPacketHeader) + ntohs(Eapol->Header.PacketBodyLength);
    if (Len > MaxEapolSize) return 1;
    Record.EapolLength = static_cast<uint16_t>(Len);
    memcpy(Record.EapolData, Eapol, Len);
    memcpy(Record.KeyMic, Key->KeyMic, 16);
    memset(&Record.EapolData[81], 0x00, 16);
    return 0;
}

static void ApM1(EapolKeyPacket* Key) {
    MsgAp = 1;
    memcpy(Record.NonceAp, Key->KeyNonce, 32);
}

static void ApM3(EapolPacket* Eapol, EapolKeyPacket* Key) {
    MsgAp = 3;
    if (MsgAp == 0) memcpy(Record.NonceAp, Key->KeyNonce, 32);
    if (EapolSource == 2) { Record.MessagePair = 2; return; }
    if (SaveEapol(Eapol, Key) != 0) return;
    EapolSource = 3;
    if (MsgSta == 2) Record.MessagePair = 3;
}

static void HandleApFrame(DataFrame* Frame, EapolPacket* Eapol, EapolKeyPacket* Key) {
    if (!IsZero(Record.MacStation, 6) && memcmp(Frame->MacHeader.Addr1, Record.MacStation, 6) != 0) return;
    if (MsgAp == 0) memcpy(Record.MacAccessPoint, Frame->MacHeader.Addr2, 6);
    if (IsZero(Key->KeyMic, 16)) ApM1(Key);
    else                          ApM3(Eapol, Key);
}

static void StaM2(EapolPacket* Eapol, EapolKeyPacket* Key) {
    MsgSta = 2;
    memcpy(Record.NonceSta, Key->KeyNonce, 32);
    if (SaveEapol(Eapol, Key) != 0) return;
    EapolSource = 2;
    if (MsgAp == 1) { Record.MessagePair = 0; return; }
}

static void StaM4(EapolPacket* Eapol, EapolKeyPacket* Key) {
    if (MsgSta == 2 && EapolSource != 0) return;
    if (MsgAp == 0) return;
    if (EapolSource == 3) { Record.MessagePair = 4; return; }
    if (SaveEapol(Eapol, Key) != 0) return;
    EapolSource = 4;
    if (MsgAp == 1) Record.MessagePair = 1;
    if (MsgAp == 3) Record.MessagePair = 5;
}

static void HandleStaFrame(DataFrame* Frame, EapolPacket* Eapol, EapolKeyPacket* Key) {
    if (IsZero(Record.MacStation, 6)) memcpy(Record.MacStation, Frame->MacHeader.Addr2, 6);
    else if (memcmp(Frame->MacHeader.Addr2, Record.MacStation, 6) != 0) return;
    if (!IsZero(Key->KeyNonce, 16)) StaM2(Eapol, Key);
    else                             StaM4(Eapol, Key);
}

namespace HccapxWriter {
    void Init(const uint8_t* Ssid, unsigned Size) {
        Record.SsidLength   = static_cast<uint8_t>(Size);
        Record.MessagePair  = 255;
        MsgAp = MsgSta = EapolSource = 0;
        memset(Record.MacAccessPoint, 0, 6);
        memset(Record.MacStation, 0, 6);
        memcpy(Record.Ssid, Ssid, Size);
    }

    HccapxRecord* Get() {
        return (Record.MessagePair == 255) ? nullptr : &Record;
    }

    void AddFrame(DataFrame* Frame) {
        EapolPacket*    Eapol = FrameDecoder::ExtractEapol(Frame);
        if (!Eapol) return;
        EapolKeyPacket* Key   = FrameDecoder::ExtractEapolKey(Eapol);
        if (!Key) return;

        if (memcmp(Frame->MacHeader.Addr2, Frame->MacHeader.Addr3, 6) == 0)
            HandleApFrame(Frame, Eapol, Key);
        else if (memcmp(Frame->MacHeader.Addr1, Frame->MacHeader.Addr3, 6) == 0)
            HandleStaFrame(Frame, Eapol, Key);
    }
}
