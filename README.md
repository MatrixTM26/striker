# Striker

> Wireless penetration testing tool built for the ESP32 microcontroller. Runs entirely on-device — no laptop required in the field.

**Author:** [MatrixTM26](https://github.com/MatrixTM26)

---

## What It Does

Striker turns an ESP32 into a self-contained WiFi auditing device. Once flashed, it creates its own management access point. Connect to it from any browser, scan nearby networks, pick a target, choose an operation mode, and launch — all from a clean dark web interface served directly from the chip.

Captured data (handshakes, PMKIDs) is available for immediate download in formats ready for offline cracking with hashcat.

---

## Operation Modes

### PMKID Capture
Connects to the target AP and extracts the PMKID from the first EAPoL-Key exchange frame. No associated clients needed on the target network. Fast and clientless.

### Handshake Capture
Records the WPA/WPA2 4-way handshake by monitoring EAPoL-Key traffic after triggering a client reconnect. Three collection methods available:

| Method | Behavior |
|--------|----------|
| RogueAp | Clones the target BSSID and SSID — forces clients to reconnect to a spoofed AP |
| Broadcast | Sends periodic broadcast deauthentication frames to all clients |
| Passive | Listens only — waits for a natural reconnect without sending any frames |

### Disrupt
Continuous deauthentication flood. Keeps all clients disconnected from the target AP for the configured duration. Three delivery modes:

| Method | Behavior |
|--------|----------|
| RogueAp | Rogue AP clone only |
| Broadcast | Broadcast deauth flood only |
| CombineAll | Both simultaneously |

---

## Hardware

| Component | Requirement |
|-----------|-------------|
| MCU | ESP32 (WROOM-32 / WROVER) |
| Flash | 4 MB minimum |
| Antenna | Built-in or external 2.4 GHz |

No additional components required.

---

## Build

Requires [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/).

```bash
. $IDF_PATH/export.sh

cd Striker

idf.py build

idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Configuration

```bash
idf.py menuconfig
```

Navigate to **NetStrike32 Configuration**.

| Key | Default | Description |
|-----|---------|-------------|
| `CONSOLE_SSID` | `NetStrike32` | SSID of the management access point |
| `CONSOLE_KEY` | `Strike32!` | WPA2 password for the management AP |
| `CONSOLE_CLIENT_LIMIT` | `1` | Max concurrent clients on the management AP |
| `RADAR_MAX_TARGETS` | `20` | Max access points stored per scan (5–50) |

`CONSOLE_CLIENT_LIMIT` controls how many devices can connect to the management AP at the same time. Default of `1` ensures only the operator has access to the interface during an engagement.

---

## Usage

1. Flash the device
2. Connect to the management AP from any device with a browser

```
SSID      : NetStrike32
Password  : Strike32!
```

3. Open `http://192.168.4.1`
4. Hit **Scan** to discover nearby access points
5. Select a target from the list
6. Choose an operation mode and method
7. Set a timeout (0 = run until manually aborted)
8. Launch

When the operation completes, the result panel appears with captured data and download links.

---

## Output Files

### PMKID

The interface generates the hashcat-ready string automatically:

```
<pmkid>*<mac_ap>*<mac_sta>*<ssid_hex>
```

Crack with:

```bash
hashcat -m 22000 hash.txt wordlist.txt
```

### Handshake — HCCAPX

```bash
hashcat -m 22000 capture.hccapx wordlist.txt
```

### Handshake — PCAP

```bash
hcxpcapngtool -o hash.hc22000 capture.pcap
hashcat -m 22000 hash.hc22000 wordlist.txt
```

---

## Project Structure

```
Striker/
├── core/
│   ├── Main.cpp                  Boot sequence
│   ├── Kconfig.projbuild         All menuconfig options
│   └── CMakeLists.txt
│
├── modules/
│   ├── RadioInterface/           WiFi driver — AP, STA, sniffer, scanner
│   ├── FrameParser/              802.11 and EAPoL frame parsing
│   ├── FrameInjector/            Raw deauthentication frame injection
│   ├── CaptureWriter/            PCAP and HCCAPX file serialization
│   ├── PacketEngine/             Attack orchestration and state machine
│   └── HttpServer/               REST API and embedded web interface
│
├── CMakeLists.txt
└── sdkconfig.defaults
```

### Module Dependency Graph

```
core
├── PacketEngine
│   ├── RadioInterface
│   ├── FrameParser → RadioInterface
│   ├── CaptureWriter → FrameParser
│   └── FrameInjector
└── HttpServer
    ├── PacketEngine
    └── CaptureWriter
```

---

## REST API

Served from `http://192.168.4.1` while connected to the management AP.

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/` | Web interface |
| `GET` | `/networks` | Scan and return AP list |
| `POST` | `/strike` | Launch operation |
| `GET` | `/status` | Poll engine state and result |
| `HEAD` | `/reset` | Reset engine to idle |
| `GET` | `/capture.pcap` | Download PCAP |
| `GET` | `/capture.hccapx` | Download HCCAPX |

**Strike request payload** (4 bytes, binary):

```
[0] TargetId   — index into last /networks response
[1] Mode       — 0=Silent  1=Handshake  2=Pmkid  3=Disrupt
[2] Method     — 0=RogueAp  1=Broadcast  2=Passive/CombineAll
[3] Duration   — seconds (0 = no timeout)
```

**Status response** (binary):

```
[0]   State       — 0=Idle  1=Active  2=Done  3=Expired
[1]   Mode        — current operation mode
[2-3] PayloadSize — byte length of following payload
[4+]  Payload     — captured data (when State=Done or Expired)
```

---

## Legal

For use only on networks you own or have explicit written authorization to test. The author is not responsible for any unauthorized or unlawful use of this tool.
