# Heltec V4 energy-optimization implementation

This generation reduces unnecessary board, display, sensor and connectivity
activity while keeping LoRa transmission at the maximum permitted by this
fork's FEM-aware output policy.

## RF policy in other roles

- Every retained Heltec V4 target requests `MAX_LORA_TX_POWER` before the
  board-specific FEM mapping.
- The existing estimated antenna-path ceiling remains 22 dBm.
- GC1109 hardware applies 11 dBm at the SX1262 input.
- KCT8103L hardware applies 9 dBm at the SX1262 input.
- Lower persisted values or client commands remain protocol-compatible but
  cannot reduce physical transmit power.
- A 20 microsecond FEM-settle guard is applied before the first LoRa preamble
  symbol, preventing transmission from beginning while the external front end
  is still changing state.
- Runtime RX profiles affect receiver gain only and never alter TX power.

The 22 dBm value is a firmware estimate based on the detected front end. It is
not a substitute for measuring conducted RF power on V4.2 and V4.3 hardware.

## Supported targets

The distribution builds ten Heltec V4 OLED targets. Solar Repeater replaces the
official standard Repeater; the other nine roles are retained.

## Solar Repeater

`heltec_v4_solar_repeater` inherits the shared repeater implementation. It starts
with the OLED off, defaults to local advertisements every thirty minutes, enables
power saving with a thirty-second initial delay, and omits PSRAM. The solar
profile uses its own battery curve, suspends operation after confirmed critical
battery readings below 3500 mV, and checks recovery every sixty seconds until
3650 mV is reached. OTA requires at least 3650 mV. Stored preferences take
precedence over new-install defaults. No headless Solar target is included.

## Common changes

- Battery readings used by UI and telemetry are cached for ten seconds.
  Safety checks and OTA qualification request a fresh ADC measurement.
- Every application yields to the ESP32 FreeRTOS idle task. The duplicate board
  loop inside the RadioLib wrapper was removed.
- The CPU remains fixed at the established 80 MHz with the currently pinned
  Arduino-ESP32 framework.
- Noise-floor calibration runs every two seconds during the first minute after
  boot or radio setup, then changes to a thirty-second cadence.
- Environmental-sensor drivers and full I2C discovery are compiled only into
  Sensor and Expansion Kit targets. Other roles retain optional GPS support but
  no longer load or scan the complete environmental-sensor catalog.
- External RTC probing is limited to roles that can use an accessory bus.
- OLED, GPS and accessory users hold independent reference-counted claims on
  VEXT. Turning the OLED off therefore releases only the OLED claim.
- The GPS UART is stopped while GPS is disabled and restarted when GPS is
  enabled.
- Production BLE and Wi-Fi targets no longer enable verbose transport logging.
- Repeater, Room Server and Sensor initialize the board and low-voltage guard
  before starting the debug serial port.
- New-install local advertisements are less frequent: thirty minutes for Solar
  Repeater and sixteen minutes for Room Server and Sensor.

## Framework limitation recorded by the audit

The pinned PlatformIO/Arduino-ESP32 toolchain uses a precompiled ESP-IDF
configuration in which dynamic frequency scaling, FreeRTOS automatic light
sleep, Bluetooth controller modem sleep and disconnected-station ESP-NOW power
management are not enabled. Application build flags cannot retroactively enable
those precompiled components.

The retained ten targets use cooperative idle and explicit peripheral and
transport power saving. Framework-managed automatic light sleep remains
unavailable, but the BLE Companion uses explicit ESP32-S3 light sleep after the
Bluetooth stack has been fully shut down.

## Connectivity policies

### Standard BLE Companion

- Advertises quickly for one minute after boot or a disconnect.
- Changes to a slower advertising interval after that minute.
- Requests short connection intervals while data is active.
- Requests longer intervals and slave latency after ten seconds without data.
- After five minutes without a connection, unregisters/frees GATT attributes and disables Bluedroid and
  the ESP32-S3 Bluetooth controller. Connected sessions are never stopped by
  this timer.
- While Bluetooth is fully down and the mesh has no pending work, enters
  explicit light sleep.
- The SX1262 remains in continuous receive; LoRa DIO1 wakes the MCU immediately.
- PRG and a bounded 20 ms maintenance timer are also configured as wake sources.
- Restoring Bluetooth from the node's Bluetooth page with a long PRG press
  rebuilds encrypted GATT without rebooting. Identity, contacts, channels, queued
  messages and persistent settings remain intact.
- The maximum TX policy is unchanged before, during and after the BLE-off state.

### Standard Wi-Fi Companion

- Enables Wi-Fi modem power saving while associated.
- Uses exponential reconnect timing instead of reconnecting every ten seconds
  indefinitely.
- Keeps the Wi-Fi transport available whenever the network is reachable.

### Standard ESP-NOW bridge

The standard bridge remains continuously available.

## RX profiles

The default remains `range`, preserving the existing boosted SX1262 receiver
and the KCT8103L external LNA. Available runtime profiles are:

- `range`: SX1262 boosted RX plus external LNA where controllable.
- `balanced`: normal SX1262 RX plus external LNA.
- `battery`: normal SX1262 RX and external-LNA bypass where controllable.

```text
get radio.rxprofile
set radio.rxprofile range
set radio.rxprofile balanced
set radio.rxprofile battery
```

GC1109 does not expose the same independently controllable external LNA path,
so the profile command reports that its external receive path is fixed. RX
profiles never lower physical transmit power.

## Selective upstream connectivity and telemetry integration

The BLE Companion also provides an optional Wi-Fi/TCP interface. On a fresh BLE
installation no SSID is configured, so the Wi-Fi radio and TCP server are not
started. The Wi-Fi Companion retains their compile-time fallback credentials.
Stored credentials take precedence over those build defaults.

```text
set wifi.ssid network-name
set wifi.pwd network-password
set wifi.enabled 1
reboot
get wifi.status
get wifi.ip
```

Credential and enable changes take effect after reboot. `set wifi.clear` removes
stored credentials and restores the build defaults. `get wifi.pwd` reports only
whether a password is configured; it does not return the password. Settings use
the existing protected Companion preferences storage.

Wi-Fi reconnect backoff and modem power saving remain in place. The BLE
Companion only enters the BLE-off light-sleep path when optional Wi-Fi is not
configured and active.

Sensor nodes support authenticated, scoped telemetry subscriptions with a
caller-supplied lifetime and nonnegative scalar LPP change thresholds. Invalid,
truncated or composite thresholds are rejected before client state is changed.
Expired subscriptions stop producing updates. Subscription state is transient:
clients must subscribe again after a reboot.

The Companion forwards unhandled binary responses, including pushed telemetry,
to the connected application. The UI receives mesh events through a listener
while retaining the local OLED page, release-to-disable action, persistent wake
suppression, calibrated battery percentage and long-PRG restoration behavior.

`get gps diag` adds observation-only GPS diagnostics. The Heltec GPS startup
suppression, UART shutdown and independent VEXT claims remain unchanged.

## Physical qualification

CI validates source invariants, native policy tests, compilation of every
environment, application/OTA images and complete merged images. It cannot
measure electrical current, RF output or packet loss.

Hardware qualification still requires:

- Current measurements in idle, RX, TX, advertising, connected and BLE-off
  light-sleep states.
- Conducted RF measurements on GC1109 and KCT8103L boards.
- BLE pairing, bonding, five-minute shutdown and PRG restoration tests on
  Android and iOS.
- Wi-Fi reconnect tests with the access point available and unavailable.
- ESP-NOW delivery tests.
- Sensor sample, transmit and subscription-expiry tests.
- A complete Solar Repeater 3.50 V / 3.65 V recovery cycle.
- A full battery-runtime comparison under controlled traffic and radio settings.

## High-power BLE Companion: explicit MCU light sleep

The BLE target forces maximum nominal 28 dBm output with a FEM-aware SX1262
input clamp. Existing lower saved/app TX settings are overridden in this target;
the RX profile is preserved. All other roles retain their existing TX policy.

Bluetooth shutdown now unregisters and frees the encrypted GATT service and
characteristics, disables Bluedroid, and disables the Bluetooth controller.
Reactivation re-registers the application, PIN/security settings and GATT service.
It preserves bonds, the node identity, application queues and persistent settings.

The companion sleeps only with the radio in RX, no queued outgoing work, Bluetooth
suspended, Wi-Fi disabled, GPS disabled, OLED off and buzzer quiet. Manual sleep
is inhibited when the USB Serial/JTAG peripheral is connected to a computer,
including a two-second disconnect/enumeration grace period. Radio DIO1 is a
high-level wake source and PRG is a low-level source. A timer bounds sleep to
20 ms. The SX1262 and FEM remain powered and are not duty-cycled. IRQ setup masks
the RadioLib interrupt, restores its rising edge afterward and polls the latched
DIO1 level to recover arrivals during that interval. The loop still yields to
FreeRTOS after sleep. Diagnostic counters distinguish successful sleeps, timer
and GPIO wakes, cumulative sleep time and the last error.

Physical qualification must distinguish MCU/timer/DIO1 and GATT tests from a
controlled over-air packet-loss test. A second MeshCore radio can send numbered
zero-hop raw packets without changing contacts or flooding other nodes. The
local diagnostic build adds a RAM-only 64-sequence collector and a two-minute
USB sleep override under `HELTEC_V4_ALLOW_USB_SLEEP_TEST`. Only armed packets
with the matching `MCRXTEST` marker and run ID are consumed by the collector,
avoiding USB writes while that peripheral sleeps. Normal traffic keeps its
usual handling. The override restores USB automatically at the end of the
window. These commands and the collector are excluded from normal firmware.

The local test commands are `set power.rxtest <hex-run-id>`,
`get power.rxtest`, `set power.rxtest stop`, and `set power.sleep rxtest`.
The 14-byte payload consists of the eight-byte marker, a little-endian 32-bit
run ID and a little-endian 16-bit sequence (0–63). Read the sequence bitmap and
sleep counters after USB recovers; compare unique receives with completed
transmissions, rather than commands accepted into the sender's queue.

Actual current, runtime and conducted RF output require external measurement.
