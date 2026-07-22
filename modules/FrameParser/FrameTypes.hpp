#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define EtherTypeEapol 0x888e

typedef enum {
    EapolEapPacket = 0,
    EapolStart,
    EapolLogoff,
    EapolKey,
    EapolEncapsulatedAsfAlert,
    EapolMka,
    EapolAnnouncementGeneric,
    EapolAnnouncementSpecific,
    EapolAnnouncementReq
} EapolPacketType;

typedef struct {
    uint8_t ProtocolVersion:2;
    uint8_t Type:2;
    uint8_t Subtype:4;
    uint8_t ToDs:1;
    uint8_t FromDs:1;
    uint8_t MoreFragments:1;
    uint8_t Retry:1;
    uint8_t PowerManagement:1;
    uint8_t MoreData:1;
    uint8_t ProtectedFrame:1;
    uint8_t HtcOrder:1;
} FrameControl;

typedef struct {
    FrameControl Control;
    uint16_t Duration;
    uint8_t Addr1[6];
    uint8_t Addr2[6];
    uint8_t Addr3[6];
    uint16_t SequenceControl;
} DataFrameMacHeader;

typedef struct {
    DataFrameMacHeader MacHeader;
    uint8_t Body[];
} DataFrame;

typedef struct {
    uint8_t SnapDsap;
    uint8_t SnapSsap;
    uint8_t Control;
    uint8_t Encapsulation[3];
} LlcSnapHeader;

typedef struct {
    uint8_t Version;
    uint8_t PacketType;
    uint16_t PacketBodyLength;
} EapolPacketHeader;

typedef struct {
    EapolPacketHeader Header;
    uint8_t PacketBody[];
} EapolPacket;

typedef struct {
    uint8_t KeyDescriptorVersion:3;
    uint8_t KeyType:1;
    uint8_t :2;
    uint8_t Install:1;
    uint8_t KeyAck:1;
    uint8_t KeyMic:1;
    uint8_t Secure:1;
    uint8_t Error:1;
    uint8_t Request:1;
    uint8_t EncryptedKeyData:1;
    uint8_t SmkMessage:1;
    uint8_t :2;
} KeyInformation;

typedef struct __attribute__((__packed__)) {
    uint8_t DescriptorType;
    KeyInformation KeyInfo;
    uint16_t KeyLength;
    uint8_t KeyReplayCounter[8];
    uint8_t KeyNonce[32];
    uint8_t KeyIv[16];
    uint8_t KeyRsc[8];
    uint8_t Reserved[8];
    uint8_t KeyMic[16];
    uint16_t KeyDataLength;
    uint8_t KeyData[];
} EapolKeyPacket;

#define KeyDataTypeByte         0xdd
#define KeyDataOuiIeee80211     0x00fac00
#define KeyDataTypePmkidKde     4

typedef struct __attribute__((__packed__)) {
    uint8_t Type;
    uint8_t Length;
    uint32_t Oui:24;
    uint32_t DataType:8;
    uint8_t Data[];
} KeyDataField;

typedef struct PmkidNode {
    uint8_t Value[16];
    struct PmkidNode* Next;
} PmkidNode;

#ifdef __cplusplus
}
#endif
