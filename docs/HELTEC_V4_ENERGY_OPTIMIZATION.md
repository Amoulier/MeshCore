# Heltec V4 energy optimization implementation

This implementation preserves the historical MeshCore `tx` meaning (SX1262
input dBm) and retains the detected-FEM estimated antenna-output ceiling of
22 dBm.

## Common changes

- The default SX1262 input is 11 dBm on new installs. That reaches the existing
  22 dBm estimate on both GC1109 and KCT8103L paths; persisted settings are not
  overwritten.
- A 20 microsecond FEM settle guard is applied before LoRa transmission begins.
- Battery measurements are cached for ten seconds for UI and telemetry. Solar
  cutoff checks and OTA qualification request a fresh measurement.
- Every application yields to the ESP32 FreeRTOS idle task. The duplicate board
  loop inside the RadioLib wrapper was removed.
- DFS is enabled where the framework supports it. Automatic light sleep is
  enabled only on targets whose transport is designed for it.
- Noise-floor calibration is fast for the first minute, then changes from a
  two-second to a thirty-second cadence.
- External RTC and environmental-sensor probing are compiled only into targets
  that request them.
- OLED VEXT ownership is reference-counted by the board/accessory and display
  independently.
- GPS UART is stopped when GPS is disabled and restarted when enabled.

## Connectivity

BLE advertises quickly for one minute after boot or disconnect, then changes to
a slower interval. Connected links use interactive parameters during transfers
and request lower-event-rate parameters after ten seconds of inactivity.

Wi-Fi enables modem sleep and uses exponential reconnect backoff. The
`wifi_low_power` variant permits automatic light sleep and a longer backoff cap.

The standard ESP-NOW bridge remains continuously reachable. The
`espnow_low_power` variant requests a 20 ms receive window every 100 ms and can
therefore miss uncoordinated packets; it is for coordinated battery deployments.

## RX profiles

The default remains `range`, preserving the existing boosted SX1262 receiver and
the KCT8103L external LNA. Runtime profiles are:

- `range`: SX1262 boosted RX plus external LNA where controllable.
- `balanced`: normal SX1262 RX plus external LNA.
- `battery`: normal SX1262 RX and external-LNA bypass where controllable.

Commands:

```text
get radio.rxprofile
set radio.rxprofile range
set radio.rxprofile balanced
set radio.rxprofile battery
```

RX profiles never reduce transmit power.

## Specialized targets

- `heltec_v4_sensor_low_power` wakes for 12 seconds every 15 minutes, powers the
  sensor rail during the wake window, samples, advertises, services queued work,
  then deep sleeps. It is not continuously reachable.
- `heltec_v4_solar_repeater_headless` removes application USB CDC and PSRAM
  initialization. Use the normal Solar Repeater when a runtime USB console is
  required.
- Low-power BLE, Wi-Fi and ESP-NOW variants retain their corresponding standard
  variants for direct A/B validation.

## Required physical validation

CI verifies compilation, policy tests and image packaging. Release qualification
still requires current and RF measurements for V4.2 and V4.3, BLE reconnect
tests, Wi-Fi/ESP-NOW traffic tests, sensor wake/sleep tests and the full
3.50/3.65 V solar recovery cycle.
