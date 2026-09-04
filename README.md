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
| `heltec_v4_repeater` | Standard OLED repeater |
| `heltec_v4_solar_repeater` | Unattended solar repeater with protective battery recovery |
| `heltec_v4_expansionkit_repeater` | Repeater with expansion-kit sensor bus |
| `heltec_v4_repeater_bridge_espnow` | ESP-NOW bridge repeater |
| `heltec_v4_room_server` | Room server |
| `heltec_v4_terminal_chat` | Terminal chat firmware |
| `heltec_v4_sensor` | Sensor node |
| `heltec_v4_kiss_modem` | USB KISS modem |

## Heltec V4 power and radio behavior

All OLED targets share these hardware-focused changes:

- Calibrated 12-bit battery sampling using 15 averaged ADC readings.
- Heltec-specific open-circuit-voltage curves and a displayed percentage limited to one point of movement per minute.
- Persistent OLED shutdown, physical VEXT control, automatic-wake suppression, and restoration by holding PRG.
- GPS disabled without powering, resetting, probing, or delaying boot; enabling it later retains VEXT while the GPS is active.
- TX/status LED suppression for unattended operation.
- Automatic GC1109/KCT8103L detection, KCT8103L LNA enabled by default, and a FEM-aware ceiling that keeps estimated output at or below 22 dBm.
- Temporary-file, backup, and rename-based persistence for identity and configuration stores, with a write guard during critical battery recovery. Explicit deletion removes the primary file together with its `.tmp` and `.bak` recovery sidecars so deleted state cannot reappear after reboot.

The existing MeshCore `tx` setting keeps its historical meaning: requested **SX1262 input power**, not antenna output. The board may lower the applied value according to the detected FEM. For example, the default request of 10 dBm remains 10 dBm on GC1109 but is limited to 9 dBm on KCT8103L.

The current Arduino-ESP32 framework used by this repository does not enable ESP-IDF dynamic frequency scaling. Heltec V4 therefore remains at the existing fixed 80 MHz CPU clock; automatic light sleep is not introduced.

### Standard profile

The standard Companion, Repeater, Room Server, Sensor, Bridge, Terminal and KISS targets use the normal MeshCore discharge policy. They receive the common ADC, OLED, GPS, LED, radio and storage improvements, but do not force the 3.50 V solar recovery latch.

### Solar Repeater profile

`heltec_v4_solar_repeater` adds an unattended low-battery policy:

- OLED defaults to persistently off on a fresh installation.
- Three consecutive readings at or below 3.50 V trigger protective sleep.
- Identity and configuration writes are blocked before shutdown.
- OLED, VEXT, GPS, SX1262 and the external FEM are placed in their low-power states.
- The node wakes only by timer every 60 seconds while recovering.
- Normal boot resumes only after the battery reaches 3.65 V.

This profile is intentionally limited to the repeater application. Use a standard Companion target when continuous BLE, USB or Wi-Fi availability is required.

## Operational commands

The existing MeshCore CLI accepts these Heltec V4 additions:

```text
get power.status
get radio.power
get radio.fem.rxgain
set radio.fem.rxgain on
set radio.fem.rxgain off
get display
set display off
set display on
```

`get power.status` reports raw millivolts, the filtered percentage and the active Standard/Solar profile. `get radio.power` reports requested SX1262 input, the value actually applied after the FEM limit, and estimated antenna output. After `set display off`, messages and ordinary button presses do not wake the OLED; hold PRG for about one second to restore it.

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
./build.sh heltec_v4_repeater heltec_v4_solar_repeater heltec_v4_companion_radio_ble
```

## Configuration before deployment

Several upstream example targets contain development defaults such as `password`, `hello`, `myssid`, and `mypwd`. Replace these values before deploying a repeater, room server, sensor, or Wi-Fi companion. Do not commit production credentials.

Confirm frequency, bandwidth, spreading factor, coding rate, requested transmit power, actual measured RF output, antenna system and regional requirements before transmitting. CI validates compilation and pure policy logic; it does not replace current-consumption, RF-power, solar-controller or full discharge/recovery tests on the final hardware installation.

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