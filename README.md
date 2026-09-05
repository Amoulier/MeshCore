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
| `heltec_v4_companion_radio_ble` | BLE Companion that remains continuously discoverable |
| `heltec_v4_companion_radio_ble_low_power` | BLE Companion that turns Bluetooth off after five disconnected minutes |
| `heltec_v4_companion_radio_usb` | USB Companion node |
| `heltec_v4_companion_radio_wifi` | Wi-Fi Companion with modem power saving |
| `heltec_v4_companion_radio_wifi_low_power` | Wi-Fi Companion that powers the radio off between reconnect attempts |
| `heltec_v4_repeater` | Standard OLED repeater |
| `heltec_v4_solar_repeater` | Unattended solar repeater with protective battery recovery |
| `heltec_v4_solar_repeater_headless` | Solar repeater without application USB CDC or PSRAM initialization |
| `heltec_v4_expansionkit_repeater` | Repeater with expansion-kit sensor bus |
| `heltec_v4_repeater_bridge_espnow` | Continuously available ESP-NOW bridge repeater |
| `heltec_v4_repeater_bridge_espnow_low_power` | ESP-NOW bridge with a five-second-on/twenty-five-second-off radio cycle |
| `heltec_v4_room_server` | Room server |
| `heltec_v4_terminal_chat` | Terminal chat firmware |
| `heltec_v4_sensor` | Always-on sensor node |
| `heltec_v4_sensor_low_power` | Twelve-second wake / fifteen-minute deep-sleep sensor profile |
| `heltec_v4_kiss_modem` | USB KISS modem |

## Heltec V4 power and radio behavior

The retained targets share these hardware-focused changes where applicable:

- Calibrated 12-bit battery sampling using 15 averaged ADC readings, with a ten-second UI/telemetry cache and uncached safety checks.
- Cooperative ESP32 idle in every role and removal of a duplicate board loop from the RadioLib wrapper.
- Fast-then-slow BLE advertising, adaptive BLE connection intervals, Wi-Fi modem saving and reconnect backoff.
- Role-specific environmental-sensor dependencies and I2C discovery instead of loading the entire sensor catalog into every firmware.
- Heltec-specific open-circuit-voltage curves and a displayed percentage limited to one point of movement per minute.
- Persistent OLED shutdown, physical VEXT control, automatic-wake suppression, and restoration by holding PRG.
- GPS disabled without powering, resetting, probing, or delaying boot; its UART is stopped while disabled and restarted when enabled.
- TX/status LED suppression for unattended operation.
- Automatic GC1109/KCT8103L detection, KCT8103L LNA enabled by default, and a FEM-aware ceiling that keeps estimated output at or below 22 dBm.
- Temporary-file, backup, and rename-based persistence for identity and configuration stores, with a write guard during critical battery recovery.
- Fast noise-floor calibration during the first minute followed by a lower-overhead thirty-second maintenance cadence.
- Less frequent new-install advertisements: sixteen minutes for standard Repeater, Room Server and Sensor; thirty minutes for Solar Repeater.

### Transmit power

The MeshCore `tx` setting keeps its historical meaning: requested **SX1262 input power**, not antenna output.

New installations now default to **11 dBm** SX1262 input instead of 10 dBm. FEM-aware limiting applies 11 dBm on GC1109 and 9 dBm on KCT8103L, producing the existing estimated 22 dBm antenna-path ceiling on either revision. Persisted `tx` values are preserved during an update. A 20 microsecond FEM-settle guard prevents the first preamble symbol from starting while the external front end is changing into TX mode.

This is a firmware estimate, not an RF certification result. Measure conducted output on the final V4.2 or V4.3 assembly before relying on the value.

### CPU and framework boundary

The CPU remains at the established fixed 80 MHz. The pinned Arduino-ESP32 framework was built without ESP-IDF dynamic frequency scaling, automatic light sleep, Bluetooth controller modem sleep, or disconnected-station ESP-NOW power management. Application flags cannot enable those precompiled features after the fact.

This release therefore uses policies that are effective with the current framework: cooperative idle, explicit Solar Repeater light sleep, Sensor deep sleep, BLE advertising/connection scheduling, Wi-Fi radio-off backoff, and an explicit ESP-NOW duty-cycle variant. It does not claim framework-level DFS or automatic light sleep.

### Standard profile

Standard Companion, Repeater, Room Server, Sensor, Bridge, Terminal and KISS targets retain continuous availability appropriate to their role. They receive the common ADC, OLED, GPS, LED, radio, storage and idle improvements but do not force the 3.50 V solar recovery latch.

### Low-power connectivity variants

- The BLE low-power target disables Bluetooth after five minutes without a connection. LoRa continues running. Restore Bluetooth locally from the Bluetooth page with a long PRG press.
- The Wi-Fi low-power target switches the Wi-Fi radio completely off between failed reconnect attempts. Backoff grows from ten seconds to fifteen minutes and resets after a successful connection.
- The ESP-NOW low-power target listens for five seconds, powers Wi-Fi/ESP-NOW off for twenty-five seconds, and repeats. Outgoing bridge traffic wakes it immediately; incoming packets during an off window are lost.
- The standard BLE, Wi-Fi and ESP-NOW targets remain available when continuous connectivity is required.

### Solar Repeater profile

`heltec_v4_solar_repeater` adds an unattended low-battery policy:

- OLED defaults to persistently off on a fresh installation.
- Power saving is enabled by default after a thirty-second startup window.
- Three consecutive readings at or below 3.50 V trigger protective sleep.
- Identity and configuration writes are blocked before shutdown.
- OLED, VEXT, GPS, SX1262 and the external FEM are placed in their low-power states.
- The node wakes only by timer every 60 seconds while recovering.
- Normal boot resumes only after the battery reaches 3.65 V.
- Every retained application calls the board-level periodic hook; CI rejects an application entry point that omits it.

Use the standard Solar Repeater when a runtime USB console is needed. The optional headless target removes application USB CDC and PSRAM initialization for unattended deployment.

### Sensor low-power profile

`heltec_v4_sensor_low_power` enables the accessory rail, samples and services queued work during a twelve-second wake window, then deep sleeps for fifteen minutes. Its OLED defaults off, periodic advertisement timer is disabled, and USB CDC/PSRAM startup features are removed. It is intentionally not continuously reachable.

## Operational commands

The existing MeshCore CLI accepts these Heltec V4 additions:

```text
get power.status
get radio.power
get radio.fem.rxgain
get radio.rxprofile
set radio.rxprofile range
set radio.rxprofile balanced
set radio.rxprofile battery
set radio.fem.rxgain on
set radio.fem.rxgain off
get display
set display off
set display on
```

`get power.status` reports raw millivolts, the filtered percentage and the active Standard/Solar profile. `get radio.power` reports requested SX1262 input, the value actually applied after the FEM limit, and estimated antenna output.

RX profiles affect reception only:

- `range`: SX1262 boosted RX plus the external LNA where controllable.
- `balanced`: normal SX1262 RX plus the external LNA.
- `battery`: normal SX1262 RX and external-LNA bypass where controllable.

## On-device OLED control

The Heltec V4 Companion does not require a computer or client-side CLI to disable the screen persistently:

1. Short-press PRG to move through the home pages until `OLED: ON` appears.
2. Hold PRG for about one second.
3. Release PRG when the screen requests it; the OLED and its claim on VEXT turn off and the preference is saved.
4. To restore the OLED, hold PRG again for about one second.

Turning off the OLED does not disable Bluetooth or LoRa. Incoming messages continue to be received without waking the screen. The restored display receives a new auto-off interval instead of switching off immediately.

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

## Firmware files

Release packages use two distinct image types:

- `-ota.bin`: application image for a compatible MeshCore update path. Do not flash it at address `0x0`.
- `-merged.bin`: complete ESP32-S3 image for a fresh installation at address `0x0`.

Keep the `-merged.bin` suffix intact because the supported flasher uses it to identify a complete image.

## Configuration before deployment

Several upstream example targets contain development defaults such as `password`, `hello`, `myssid`, and `mypwd`. Replace these values before deploying a repeater, room server, sensor, or Wi-Fi companion. Do not commit production credentials.

Confirm frequency, bandwidth, spreading factor, coding rate, requested transmit power, actual measured RF output, antenna system and regional requirements before transmitting. CI validates compilation and pure policy logic; it does not replace current-consumption, RF-power, reconnection, packet-delivery, solar-controller or full discharge/recovery tests on the final hardware installation.

## Project direction

Changes in this fork must be directly useful to the supported Heltec V4 OLED hardware. The principal areas of work are:

- Battery measurement and low-voltage reliability.
- OLED and peripheral power control.
- GPS power behavior on the expansion kit.
- ESP32-S3 power consumption without compromising standard-role connectivity.
- GC1109 and KCT8103L front-end behavior.
- Reliable Companion, Repeater, Sensor and unattended-node operation.

General MeshCore protocol improvements should normally be contributed to the upstream project first. Upstream fixes can then be synchronized into this hardware-focused fork.

## Branch and upstream policy

`main` is the maintained Heltec V4-only branch. The upstream source is:

```text
https://github.com/meshcore-dev/MeshCore
```

When synchronizing upstream changes, retain only the shared code required by the Heltec V4 targets and audit any change touching ESP32 power, RadioLib, display, GPS, storage, or the LoRa front end.

## Validation details

See [`docs/HELTEC_V4_ENERGY_OPTIMIZATION.md`](docs/HELTEC_V4_ENERGY_OPTIMIZATION.md) for the implementation matrix, availability trade-offs and physical qualification requirements.

## License

MeshCore is distributed under the MIT License. See [`license.txt`](license.txt). Original MeshCore authors and contributors retain attribution for upstream work.
