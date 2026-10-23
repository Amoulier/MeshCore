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

The distribution contains ten Heltec V4 OLED targets, replacing the official standard Repeater with this fork's Solar Repeater.

| PlatformIO environment | Purpose |
| --- | --- |
| `heltec_v4_companion_radio_ble` | BLE Companion that fully powers Bluetooth down after five disconnected minutes and light-sleeps while LoRa remains available |
| `heltec_v4_companion_radio_usb` | USB Companion node |
| `heltec_v4_companion_radio_wifi` | Wi-Fi Companion with modem power saving |
| `heltec_v4_solar_repeater` | Solar repeater with battery recovery and power saving |
| `heltec_v4_expansionkit_repeater` | Repeater with expansion-kit sensor bus |
| `heltec_v4_repeater_bridge_espnow` | Continuously available ESP-NOW bridge repeater |
| `heltec_v4_room_server` | Room server |
| `heltec_v4_terminal_chat` | Terminal chat firmware |
| `heltec_v4_sensor` | Always-on sensor node |
| `heltec_v4_kiss_modem` | USB KISS modem |

## Heltec V4 power and radio behavior

The retained targets share these hardware-focused changes where applicable:

- Calibrated 12-bit battery sampling using 15 averaged ADC readings, with a ten-second UI/telemetry cache and uncached safety checks.
- Cooperative ESP32 idle in every role and removal of a duplicate board loop from the RadioLib wrapper.
- Fast-then-slow BLE advertising, adaptive BLE connection intervals, full Bluetooth-controller shutdown after the disconnected timeout, and explicit BLE-off light sleep.
- Wi-Fi modem saving and reconnect backoff.
- Role-specific environmental-sensor dependencies and I2C discovery instead of loading the entire sensor catalog into every firmware.
- Heltec-specific open-circuit-voltage curves and a displayed percentage limited to one point of movement per minute.
- Persistent OLED shutdown, physical VEXT control, automatic-wake suppression, and restoration by holding PRG.
- GPS disabled without powering, resetting, probing, or delaying boot; its UART is stopped while disabled and restarted when enabled.
- TX/status LED suppression for unattended operation.
- Automatic GC1109/KCT8103L detection, KCT8103L LNA enabled by default, and a FEM-aware estimated output ceiling of 28 dBm for high-power BLE and 22 dBm for other roles.
- Maximum physical TX is locked across every Heltec V4 role; lower saved values or client commands cannot reduce it.
- Temporary-file, backup, and rename-based persistence for identity and configuration stores, with a write guard during critical battery recovery.
- Fast noise-floor calibration during the first minute followed by a lower-overhead thirty-second maintenance cadence.
- Less frequent new-install advertisements: thirty minutes for Solar Repeater and sixteen minutes for Room Server and Sensor.

### High-power BLE Companion sleep and transmit policy

`heltec_v4_companion_radio_ble` now targets the amplified high-power Heltec V4.
It forces the nominal maximum output of 28 dBm using the detected FEM gain table
(21 dBm SX1262 input on KCT8103L). Lower saved or app-requested TX settings do not
reduce this target's output. Other firmware roles retain their 22 dBm estimated
ceiling. These are firmware table estimates, not measured antenna output.

After five disconnected minutes, BLE releases its GATT objects and suspends both
the host and controller. PRG can enable it again without rebooting or discarding
queued LoRa messages. The processor can then enter light sleep in windows of at
most 20 ms while the SX1262 and FEM stay in continuous RX. DIO1 and PRG wake it;
a timer services background tasks. Pending transmissions, Wi-Fi, GPS, the lit
OLED, buzzer activity and an attached computer's USB connection prevent sleep.
The RX boost and LNA settings are preserved.

Local diagnostic commands are `get power.sleep`, `get power.runtime`,
`set power.sleep on`, `set power.sleep off`, `set bluetooth on`, and
`set bluetooth off`. Changes to these runtime controls are not persisted.
USB sleep override and synthetic IRQ diagnostics are compiled only into the
local test build, never the normal firmware target.

### Transmit power in other roles

The MeshCore `tx` setting historically represents requested **SX1262 input power**, not antenna output. In this fork, every Heltec V4 target now forces that request to the maximum before the board-specific FEM mapping.

In the other roles, the physical transmit path stays at the existing FEM-aware estimated **22 dBm antenna-path ceiling** at all times:

- V4.2 / GC1109 applies 11 dBm at the SX1262 input.
- V4.3 / KCT8103L applies 9 dBm at the SX1262 input.

Lower persisted values or client commands remain accepted for protocol compatibility, but they cannot reduce physical transmit power. A 20 microsecond FEM-settle guard prevents the first preamble symbol from starting while the external front end is changing into TX mode.

This is a firmware estimate, not an RF certification result. Measure conducted output on the final V4.2 or V4.3 assembly before relying on the value.

### CPU and framework boundary

The CPU remains at the established fixed 80 MHz. The pinned Arduino-ESP32 framework was built without ESP-IDF dynamic frequency scaling, framework-managed automatic light sleep, Bluetooth controller modem sleep, or disconnected-station ESP-NOW power management. Application flags cannot enable those precompiled features after the fact.

The retained targets use cooperative idle, BLE advertising/connection scheduling, Wi-Fi modem power saving and reconnect backoff. The BLE Companion additionally uses explicit ESP32-S3 light sleep only after Bluedroid and the Bluetooth controller have been disabled. The SX1262 remains in continuous receive and wakes the MCU through DIO1; PRG and a bounded 20 ms maintenance timer are also wake sources.

### Standard profile

Standard Companion, Room Server, Sensor, Bridge, Terminal and KISS targets retain the continuous availability appropriate to their role. They receive the common ADC, OLED, GPS, LED, radio, storage and idle improvements.

After five minutes without a BLE connection, the Companion unregisters/frees its GATT attributes and disables Bluedroid and the ESP32-S3 Bluetooth controller. It then light-sleeps whenever the mesh is idle while preserving continuous LoRa reception. To restore Bluetooth, open the node's Bluetooth page and hold PRG; the node rebuilds its encrypted GATT service without rebooting, preserving identity, contacts, channels, queued messages and saved settings. Active BLE connections are never stopped by the timer.

BLE Companion builds also support optional Wi-Fi configured through the Companion CLI. Wi-Fi stays off without credentials; saved network changes apply after reboot. See the connectivity and telemetry integration section in `docs/HELTEC_V4_ENERGY_OPTIMIZATION.md`.

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

`get power.status` reports raw millivolts, the filtered percentage and the active power profile. `get radio.power` reports the maximum request presented to the board, the value actually applied after the FEM limit, and the estimated antenna output.

RX profiles affect reception only and never alter TX power:

- `range`: SX1262 boosted RX plus the external LNA where controllable.
- `balanced`: normal SX1262 RX plus the external LNA.
- `battery`: normal SX1262 RX and external-LNA bypass where controllable.

## On-device OLED control

The Heltec V4 Companion does not require a computer or client-side CLI to disable the screen persistently:

1. Short-press PRG to move through the home pages until `OLED: ON` appears.
2. Hold PRG for about one second.
3. Release PRG when the screen requests it; the OLED and its claim on VEXT turn off and the preference is saved.
4. To restore the OLED, hold PRG again for about one second.

Turning off the OLED does not immediately disable Bluetooth or LoRa. Incoming messages continue to be received without waking the screen. The restored display receives a new auto-off interval instead of switching off immediately.

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
./build.sh heltec_v4_solar_repeater heltec_v4_companion_radio_ble
```

## Firmware files

Release packages use two distinct image types:

- `-ota.bin`: application image for a compatible MeshCore update path. Do not flash it at address `0x0`.
- `-merged.bin`: complete ESP32-S3 image for a fresh installation at address `0x0`.

Keep the `-merged.bin` suffix intact because the supported flasher uses it to identify a complete image.

## Configuration before deployment

Several upstream example targets contain development defaults such as `password`, `hello`, `myssid`, and `mypwd`. Replace these values before deploying a repeater, room server, sensor, or Wi-Fi companion. Do not commit production credentials.

Confirm frequency, bandwidth, spreading factor, coding rate, actual measured RF output, antenna system and regional requirements before transmitting. This fork intentionally does not permit reducing physical TX power through the normal `tx` setting. CI validates compilation and pure policy logic; it does not replace current-consumption, RF-power, reconnection, packet-delivery or full battery-discharge tests on the final hardware installation.

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