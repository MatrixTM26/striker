# Striker

> ESP32-based WiFi penetration testing tool — rebuilt in modern C++ with a clean modular architecture and upgraded web interface.

**Author:** [github.com/MatrixTM26](https://github.com/MatrixTM26)

---

## Overview

Striker is a wireless security assessment tool running on the ESP32 microcontroller. It exposes a browser-based interface over a local management AP, allowing you to scan nearby networks, select a target, and execute different attack modes — all without needing a laptop in the field.

This project is a full C++ rewrite and architectural redesign of the original [esp32-wifi-penetration-tool](https://github.com/risinek/esp32-wifi-penetration-tool) by risinek. The core logic has been restructured into namespaced C++ modules, all identifiers follow PascalCase convention, and the web UI has been completely rebuilt.

---

## Features

- Scans and lists nearby access points with signal strength
- PMKID capture — clientless, no connected stations required
- WPA/WPA2 4-way handshake capture with three collection methods
- Deauthentication flood (DoS) in three delivery modes
- Browser-based interface served directly from the device
- Real-time status polling with progress ring and attack state display
- PCAP and HCCAPX file download — hashcat-ready output
- Configurable timeout per operation
- Management AP with WPA2 authentication

---

## Attack Modes

| Mode | Description |
|------|-------------|
| `ModePmkid` | Connects to the target AP and extracts PMKID from the first EAPoL-Key frame. No clients needed. |
| `ModeHandshake` | Captures the WPA 4-way handshake by listening after forcing a client reconnect via deauth. |
| `ModeDisrupt` | Continuous deauthentication flood — disrupts all client sessions on the target AP. |
| `ModeSilent` | Reserved — passive mode, not yet implemented. |

### Handshake & Disrupt Methods

| Method | Description |
|--------|-------------|
| `0` — RogueAp | Clones the target BSSID and SSID to force reconnect to a rogue AP. |
| `1` — Broadcast | Sends periodic broadcast deauth frames to all clients. |
| `2` — CombineAll | *(Disrupt only)* Both RogueAp and Broadcast simultaneously. |

---

## Architecture

```
NetStrike32/
├── core/
│   ├── Main.cpp                  Entry point — boots AP, engine, server
│   ├── CMakeLists.txt
│   └── Kconfig.projbuild         All menuconfig options
│
├── modules/
│   ├── RadioInterface/           ESP32 WiFi APSTA, sniffer, scanner
│   │   ├── RadioInterface.hpp
│   │   ├── RadioCore.cpp
│   │   ├── RadioScanner.cpp
│   │   └── RadioSniffer.cpp
│   │
│   ├── FrameParser/              802.11 / EAPoL / PMKID frame analysis
│   │   ├── FrameTypes.hpp        All 802.11 and EAPoL struct definitions
│   │   ├── FrameParser.hpp/cpp   Event-driven frame dispatcher
│   │   └── FrameDecoder.hpp/cpp  Frame extraction and PMKID parsing
│   │
│   ├── FrameInjector/            Raw 802.11 frame injection (deauth)
│   │   ├── FrameInjector.hpp
│   │   └── FrameInjector.cpp
│   │
│   ├── CaptureWriter/            PCAP and HCCAPX serialization
│   │   ├── CaptureWriter.hpp
│   │   ├── PcapWriter.cpp        std::vector-backed PCAP builder
│   │   └── HccapxWriter.cpp      Hashcat HCCAPX record builder
│   │
│   ├── PacketEngine/             Attack orchestration and state machine
│   │   ├── PacketEngine.hpp/cpp  Engine lifecycle, timer, event wiring
│   │   ├── EventBus.hpp          Shared event base (breaks circular dep)
│   │   ├── StrikeMethod.hpp/cpp  Broadcast and RogueAp primitives
│   │   ├── StrikePmkid.hpp/cpp   PMKID attack flow
│   │   ├── StrikeHandshake.hpp/cpp  Handshake capture flow
│   │   └── StrikeDisrupt.hpp/cpp    Deauth flood flow
│   │
│   └── HttpServer/               HTTP REST API and web UI
│       ├── HttpServer.hpp/cpp
│       └── pages/page_index.h    Gzip-compressed HTML UI (embedded)
│
├── CMakeLists.txt
└── sdkconfig.defaults
```

### Dependency Graph

```
core
├── PacketEngine
│   ├── RadioInterface
│   ├── FrameParser
│   │   └── RadioInterface
│   ├── CaptureWriter
│   │   └── FrameParser
│   └── FrameInjector
└── HttpServer
    ├── PacketEngine
    └── CaptureWriter
```

No circular dependencies. `EventBus.hpp` decouples `PacketEngine` from `HttpServer` at the header level.

---

## REST API

All endpoints are served from `192.168.4.1` on the management AP.

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/` | Web interface (gzip HTML) |
| `GET` | `/networks` | Scan and return AP list (binary, 40 bytes/record) |
| `POST` | `/strike` | Launch attack (`StrikeRequest` binary payload) |
| `GET` | `/status` | Poll engine state and result payload |
| `HEAD` | `/reset` | Reset engine to idle |
| `GET` | `/capture.pcap` | Download captured PCAP file |
| `GET` | `/capture.hccapx` | Download HCCAPX file for hashcat |

### StrikeRequest Binary Format

```
Byte 0  — TargetId   (index from /networks scan)
Byte 1  — Mode       (0=Silent 1=Handshake 2=Pmkid 3=Disrupt)
Byte 2  — Method     (0=RogueAp 1=Broadcast 2=CombineAll)
Byte 3  — Duration   (seconds, 0 = unlimited)
```

### Status Response Format

```
Byte 0  — State      (0=Idle 1=Active 2=Done 3=Expired)
Byte 1  — Mode       (active operation mode)
Byte 2-3 — PayloadSize
Byte 4+ — Payload    (PMKID data or EAPoL frames, if State=Done/Expired)
```

---

## Requirements

| Component | Version |
|-----------|---------|
| ESP-IDF | `v5.x` |
| Target | ESP32 (WROOM / WROVER) |
| Flash | 4 MB minimum |
| Python | 3.8+ (for IDF toolchain) |

---

## Build & Flash

```bash
# Set up ESP-IDF environment
. $IDF_PATH/export.sh

# Clone / open project
cd NetStrike32

# (Optional) configure AP credentials, scan limits, etc.
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

Default management AP after flashing:

```
SSID      : NetStrike32
Password  : Strike32!
IP        : 192.168.4.1
```

Open `http://192.168.4.1` in any browser connected to the AP.

---

## Configuration

All options are available under `idf.py menuconfig` → **NetStrike32 Configuration**.

| Key | Default | Description |
|-----|---------|-------------|
| `MGMT_AP_SSID` | `NetStrike32` | Management AP network name |
| `MGMT_AP_PASSWORD` | `Strike32!` | Management AP WPA2 password |
| `MGMT_AP_MAX_CONNECTIONS` | `1` | Max concurrent clients on management AP |
| `SCAN_MAX_AP` | `20` | Max APs returned per scan (5–50) |

---

## Post-Capture Usage

### PMKID — hashcat mode 22000

```bash
hashcat -m 22000 hash.txt wordlist.txt
```

The web interface formats the PMKID string automatically:
```
<pmkid>*<mac_ap>*<mac_sta>*<ssid_hex>
```

### Handshake — HCCAPX

```bash
hashcat -m 22000 capture.hccapx wordlist.txt
```

### Handshake — PCAP (via hcxtools)

```bash
hcxpcapngtool -o hash.hc22000 capture.pcap
hashcat -m 22000 hash.hc22000 wordlist.txt
```

---

## Legal Notice

This tool is intended strictly for authorized security testing, research, and educational purposes on networks you own or have explicit written permission to test. Unauthorized use against networks you do not own is illegal in most jurisdictions. The author assumes no liability for misuse.

---

## Credits

Original concept and C implementation by [risinek](https://github.com/risinek/esp32-wifi-penetration-tool).
C++ rewrite, module redesign, and UI overhaul by [MatrixTM26](https://github.com/MatrixTM26).
