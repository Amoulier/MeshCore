# Heltec V4 energy-optimization implementation

This generation reduces unnecessary board, display, sensor and connectivity
activity without lowering LoRa transmit power. It preserves the historical
MeshCore meaning of `tx`: requested **SX1262 input dBm**, not measured antenna
output.

## RF policy

- New installations default to 11 dBm SX1262 input instead of 10 dBm.
- On GC1109 hardware, 11 dBm input maps to the existing estimate of 22 dBm at
  the antenna path.
- On KCT8103L hardware, the board limits the SX1262 input to 9 dBm, which maps
  to the same existing 22 dBm estimate.
- Persisted `tx` values are not silently overwritten during an update.
- A 20 microsecond FEM-settle guard is applied before the first LoRa preamble
  symbol, preventing the transmission from beginning while the external front
  end is still changing state.
- Runtime RX profiles change receiver gain only; they never reduce TX power.

The 22 dBm value is a firmware estimate based on the detected front end. It is
not a substitute for measuring conducted RF power on V4.2 and V4.3 hardware.

## Supported targets

The distribution builds ten Heltec V4 OLED targets. Solar Repeater replaces the official standard Repeater; the other nine roles are retained.

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
- New-install local advertisements are less frequent: thirty minutes for Solar Repeater and sixteen minutes for
  Room Server and Sensor.

## Framework limitation recorded by the audit

The pinned PlatformIO/Arduino-ESP32 toolchain uses a precompiled ESP-IDF
configuration in which dynamic frequency scaling, FreeRTOS automatic light
sleep, Bluetooth controller modem sleep and disconnected-station ESP-NOW power
management are not enabled. Application build flags cannot retroactively enable
those precompiled components.

The retained ten targets use cooperative idle and explicit peripheral and
transport power saving. DFS and automatic light sleep remain unavailable with
the pinned framework.

## Connectivity policies

### Standard BLE Companion

- Advertises quickly for one minute after boot or a disconnect.
- Changes to a slower advertising interval after that minute.
- Requests short connection intervals while data is active.
- Requests longer intervals and slave latency after ten seconds without data.
- Remains enabled and discoverable indefinitely.

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
so the profile command reports that its external receive path is fixed.

## Selective upstream connectivity and telemetry integration

The BLE Companion also provides an optional Wi-Fi/TCP interface. On a
fresh BLE installation no SSID is configured, so the Wi-Fi radio and TCP server
are not started. The Wi-Fi Companion retains their compile-time fallback
credentials. Stored credentials take precedence over those build defaults.

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

Wi-Fi reconnect backoff and modem power saving remain in place. BLE and
ESP-NOW remain continuously available in their corresponding retained targets.

Sensor nodes support authenticated, scoped telemetry subscriptions with a
caller-supplied lifetime and nonnegative scalar LPP change thresholds. Invalid,
truncated or composite thresholds are rejected before client state is changed.
Expired subscriptions stop producing updates. Subscription state is transient:
clients must subscribe again after a reboot.

The Companion forwards unhandled binary responses, including pushed telemetry,
to the connected application. The UI now receives mesh events through a listener
while retaining the local OLED page, release-to-disable action, persistent wake
suppression, calibrated battery percentage and long-PRG restoration behavior.

`get gps diag` adds observation-only GPS diagnostics. The Heltec GPS startup
suppression, UART shutdown and independent VEXT claims remain unchanged.

### Physical qualification

CI validates native policy tests, compilation of every environment, generation
of application/OTA images and generation of complete merged images. It cannot
measure electrical current, RF output or packet loss.

Hardware qualification still requires:

- Current measurements in idle, RX, TX, advertising, connected and sleep
  states.
- Conducted RF measurements on GC1109 and KCT8103L boards.
- BLE pairing, bonding and reconnection tests on Android and iOS.
- Wi-Fi reconnect tests with the access point available and unavailable.
- ESP-NOW delivery tests across active and inactive windows.
- Sensor sample, transmit and subscription-expiry tests.
