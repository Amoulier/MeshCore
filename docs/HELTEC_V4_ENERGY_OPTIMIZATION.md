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

## Common changes

- Battery readings used by UI and telemetry are cached for ten seconds. Solar
  cut-off decisions, boot recovery and OTA qualification always request a fresh
  ADC measurement.
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
- New-install local advertisements are less frequent: sixteen minutes for the
  standard Repeater, Room Server and Sensor, and thirty minutes for Solar
  Repeater.

## Framework limitation recorded by the audit

The pinned PlatformIO/Arduino-ESP32 toolchain uses a precompiled ESP-IDF
configuration in which dynamic frequency scaling, FreeRTOS automatic light
sleep, Bluetooth controller modem sleep and disconnected-station ESP-NOW power
management are not enabled. Application build flags cannot retroactively enable
those precompiled components.

For that reason, this release does **not** claim DFS or automatic light sleep.
The corresponding low-power variants use explicit policies that work with the
current toolchain instead of depending on unavailable framework options. Solar
Repeater continues to use the existing explicit LoRa-DIO/timer light-sleep
path, and the low-power Sensor uses deep sleep.

## Connectivity policies

### Standard BLE Companion

- Advertises quickly for one minute after boot or a disconnect.
- Changes to a slower advertising interval after that minute.
- Requests short connection intervals while data is active.
- Requests longer intervals and slave latency after ten seconds without data.
- Remains enabled and discoverable indefinitely.

### Low-power BLE Companion

`heltec_v4_companion_radio_ble_low_power` includes all standard BLE behavior,
then disables Bluetooth completely after five minutes without a connection.
LoRa and the node continue operating. Restore Bluetooth locally from the
Bluetooth page on the Heltec display with a long PRG press. This target is not
appropriate when unattended remote BLE discovery must remain continuous.

### Standard Wi-Fi Companion

- Enables Wi-Fi modem power saving while associated.
- Uses exponential reconnect timing instead of reconnecting every ten seconds
  indefinitely.
- Keeps the Wi-Fi transport available whenever the network is reachable.

### Low-power Wi-Fi Companion

`heltec_v4_companion_radio_wifi_low_power` uses maximum modem power saving and
powers Wi-Fi completely off between failed reconnect attempts. Backoff grows
from ten seconds to a maximum of fifteen minutes and resets after a successful
connection. The Companion cannot accept TCP connections while its Wi-Fi radio
is suspended.

### Standard ESP-NOW bridge

The standard bridge remains continuously available.

### Low-power ESP-NOW bridge

`heltec_v4_repeater_bridge_espnow_low_power` explicitly powers ESP-NOW/Wi-Fi on
for five seconds and off for twenty-five seconds. Valid incoming or successful
outgoing traffic extends the active window. Outgoing MeshCore bridge traffic
wakes ESP-NOW immediately.

Inbound ESP-NOW packets sent during the twenty-five-second off window are lost.
This profile is therefore experimental and intended only for coordinated or
repeating senders that tolerate the documented receive windows.

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

## Specialized targets

- `heltec_v4_sensor_low_power` powers its sensor rail, samples and services
  queued work during a twelve-second wake window, then deep sleeps for fifteen
  minutes. It is not continuously reachable. Its USB CDC and PSRAM startup
  features are disabled.
- `heltec_v4_solar_repeater` enables the existing LoRa wake-capable power-saving
  path by default after a thirty-second startup window. Its 3.50 V shutdown and
  3.65 V recovery thresholds are unchanged.
- `heltec_v4_solar_repeater_headless` additionally removes application USB CDC
  and PSRAM initialization for unattended deployment.
- Standard variants remain available beside every connectivity variant that
  intentionally trades availability for energy.

## Validation boundary

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
- Sensor wake, sample, transmit and deep-sleep tests.
- A complete Solar Repeater 3.50 V / 3.65 V recovery cycle with the final cell,
  charger and panel.
