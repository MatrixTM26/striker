#include "FrameDecoder.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "arpa/inet.h"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_wifi_types.h"

ESP_EVENT_DEFINE_BASE(FrameParserEvents);

static const char* Tag = "FrameDecoder";

static PmkidNode* ParsePmkidFromKeyData(uint8_t* KeyData, uint16_t Length) {
    uint8_t* Cursor = KeyData;
    uint8_t* End    = KeyData + Length;
    PmkidNode* Head = nullptr;

    do {
        auto* Field = reinterpret_cast<KeyDataField*>(Cursor);
        if (Field->Type != KeyDataTypeByte) goto Next;
        if (ntohl(Field->Oui) != KeyDataOuiIeee80211) goto Next;
        if (Field->DataType != KeyDataTypePmkidKde) goto Next;
        {
            auto* Node = static_cast<PmkidNode*>(malloc(sizeof(PmkidNode)));
            Node->Next = Head;
            Head = Node;
            for (unsigned i = 0; i < 16; i++) {
                Node->Value[i] = Field->Data[i];
                printf("%02x", Node->Value[i]);
            }
            printf("\n");
        }
        Next:
        Cursor = Field->Data + Field->Length - 4 + 1;
    } while (Cursor < End);

    return Head;
}

namespace FrameDecoder {
    bool BssidMatches(wifi_promiscuous_pkt_t* Frame, uint8_t* Bssid) {
        auto* Mac = reinterpret_cast<DataFrameMacHeader*>(Frame->payload);
        return memcmp(Mac->Addr3, Bssid, 6) == 0;
    }

    EapolPacket* ExtractEapol(DataFrame* Frame) {
        uint8_t* Buf = Frame->Body;
        if (Frame->MacHeader.Control.ProtectedFrame == 1) return nullptr;
        if (Frame->MacHeader.Control.Subtype > 7) Buf += 2;
        Buf += sizeof(LlcSnapHeader);
        if (ntohs(*reinterpret_cast<uint16_t*>(Buf)) == EtherTypeEapol) {
            Buf += 2;
            return reinterpret_cast<EapolPacket*>(Buf);
        }
        return nullptr;
    }

    EapolKeyPacket* ExtractEapolKey(EapolPacket* Eapol) {
        if (Eapol->Header.PacketType != EapolKey) return nullptr;
        return reinterpret_cast<EapolKeyPacket*>(Eapol->PacketBody);
    }

    PmkidNode* ExtractPmkid(EapolKeyPacket* EapolKey) {
        if (EapolKey->KeyDataLength == 0) return nullptr;
        if (EapolKey->KeyInfo.EncryptedKeyData == 1) return nullptr;
        return ParsePmkidFromKeyData(EapolKey->KeyData, ntohs(EapolKey->KeyDataLength));
    }
}
