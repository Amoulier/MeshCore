# MeshCore for Heltec V4

[![Heltec V4 CI](https://github.com/Amoulier/MeshCore/actions/workflows/heltec-v4-ci.yml/badge.svg)](https://github.com/Amoulier/MeshCore/actions/workflows/heltec-v4-ci.yml)

A focused, unofficial MeshCore firmware fork for the **Heltec WiFi LoRa 32 V4 OLED** family, developed primarily for the **Heltec V4.3 OLED**.

This repository intentionally supports one hardware family. It keeps the MeshCore protocol core and the shared application code required to build Heltec V4 firmware, while removing unrelated boards, architectures, prebuilt binaries, documentation sites, and multi-board automation.

> This is an independent hardware-focused fork of [meshcore-dev/MeshCore](https://github.com/meshcore-dev/MeshCore). It is not an official MeshCore distribution.

## Hardware scope

Supported hardware:

- Heltec WiFi LoRa 32 V4 OLED using `variants/heltec_v4`.
- Heltec V4.2 boards with the GC1109 front end.
- Heltec V4.3 boards with the KCT8103L front end.
- The Heltec V4 expansion kit when using the expansion-kit firmware target.

Not included:

- Heltec V4 TFT firmware.
- Heltec V4 R8 firmware.
- Other Heltec products.
- nRF52, RP2040, STM32, ESP32-C6, or unrelated ESP32 hardware.

The shared MeshCore protocol implementation, ESP32 support, cryptography, tests, and application examples remain because the supported Heltec V4 targets depend on them. Their presence does not indicate support for additional boards.

## Available firmware targets

| PlatformIO environment | Purpose |
| --- | --- |
| `heltec_v4_companion_radio_ble` | Bluetooth companion node |
| `heltec_v4_companion_radio_usb` | USB companion node |
| `heltec_v4_companion_radio_wifi` | Wi-Fi companion node |
| `heltec_v4_repeater` | OLED repeater |
| `heltec_v4_expansionkit_repeater` | Repeater with expansion-kit sensor bus |
| `heltec_v4_repeater_bridge_espnow` | ESP-NOW bridge repeater |
| `heltec_v4_room_server` | Room server |
| `heltec_v4_terminal_chat` | Terminal chat firmware |
| `heltec_v4_sensor` | Sensor node |
| `heltec_v4_kiss_modem` | USB KISS modem |

## Building

Install PlatformIO, clone the repository, and select an explicit environment:

```bash
python -m pip install --upgrade platformio
git clone https://github.com/Amoulier/MeshCore.git
cd MeshCore
pio run -e heltec_v4_companion_radio_ble
```

To compile every supported Heltec V4 target:

```bash
./build.sh
```

The build script also accepts one or more environment names:

```bash
./build.sh heltec_v4_repeater heltec_v4_companion_radio_ble
```

## Configuration before deployment

Several upstream example targets contain development defaults such as `password`, `hello`, `myssid`, and `mypwd`. Replace these values before deploying a repeater, room server, sensor, or Wi-Fi companion. Do not commit production credentials.

The default radio parameters remain inherited from MeshCore. Confirm frequency, bandwidth, spreading factor, coding rate, transmit power, and regional requirements before transmitting.

## Project direction

Changes in this fork must be directly useful to the supported Heltec V4 OLED hardware. The principal areas of work are:

- Battery measurement and low-voltage reliability.
- OLED and peripheral power control.
- GPS power behavior on the expansion kit.
- ESP32-S3 power consumption without compromising connectivity.
- GC1109 and KCT8103L front-end behavior.
- Reliable companion, repeater, and unattended-node operation.

General MeshCore protocol improvements should normally be contributed to the upstream project first. Upstream fixes can then be synchronized into this hardware-focused fork.

## Branch and upstream policy

`main` is the maintained Heltec V4-only branch. The upstream source is:

```text
https://github.com/meshcore-dev/MeshCore
```

When synchronizing upstream changes, retain only the shared code required by the Heltec V4 targets and audit any change touching ESP32 power, RadioLib, display, GPS, storage, or the LoRa front end.

## License

MeshCore is distributed under the MIT License. See [`license.txt`](license.txt). Original MeshCore authors and contributors retain attribution for upstream work.