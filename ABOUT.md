# About MeshCore for Heltec V4

**MeshCore for Heltec V4** is an independent, unofficial, hardware-focused MeshCore firmware distribution maintained exclusively for the **Heltec WiFi LoRa 32 V4 OLED** family.

This fork keeps the MeshCore protocol and the shared application code required by Heltec V4 firmware while concentrating development on reliable Companion, Repeater and unattended solar-node operation. It is developed primarily on the **Heltec V4.3 OLED**, while retaining support for compatible V4.2 hardware.

> This repository is derived from `meshcore-dev/MeshCore`, but it is maintained independently and must not be represented as an official MeshCore release.

## Supported hardware

The maintained hardware scope is:

- Heltec WiFi LoRa 32 V4.2 OLED with the **GC1109** RF front end.
- Heltec WiFi LoRa 32 V4.3 OLED with the **KCT8103L** RF front end.
- The Heltec V4 expansion kit when used by a retained expansion-kit target.
- ESP32-S3, SX1262, SSD1306 OLED, GPS, Bluetooth, USB, Wi-Fi and ESP-NOW functionality required by those targets.

This fork does **not** support Heltec V4 TFT, Heltec V4 R8, other Heltec devices, or unrelated ESP32, ESP32-C6, nRF52, RP2040 and STM32 boards.

Shared protocol, cryptography, storage, ESP32 support and tests remain only where a supported Heltec V4 target depends on them. Their presence does not indicate support for additional hardware.

## Project purpose

The project focuses on four practical goals:

1. Improve Heltec V4 power behavior without sacrificing normal LoRa, Bluetooth, USB or Wi-Fi operation.
2. Make Companion and Repeater firmware dependable for daily and unattended use.
3. Protect node identity and configuration against low-voltage shutdowns and interrupted flash writes.
4. Provide hardware-specific controls directly on the node whenever relying on a computer or an unavailable mobile-app control would be impractical.

## Firmware roles

The repository builds Heltec V4 firmware for:

- Bluetooth, USB and Wi-Fi Companion nodes.
- Standard Repeater and Solar Repeater nodes.
- Expansion-kit and ESP-NOW bridge Repeaters.
- Room Server, Terminal Chat, Sensor and USB KISS modem roles.

The complete environment list and build commands are documented in [`README.md`](README.md).

## Heltec V4-specific improvements

The maintained firmware adds or strengthens:

- Calibrated 12-bit battery sampling with averaged readings, Heltec-specific discharge curves and a stabilized displayed percentage.
- Persistent OLED shutdown, automatic-wake suppression and physical VEXT management where the rail is not required by another peripheral.
- GPS-disabled boot that avoids powering, resetting, probing or delaying for an unused GPS.
- TX and status LED suppression for lower unattended power use.
- Automatic GC1109/KCT8103L detection, KCT8103L receive-gain control and FEM-aware transmit-power limits.
- Safer identity and configuration persistence using temporary files, verified recovery copies and write blocking during critical battery recovery.
- A dedicated Solar Repeater policy with low-voltage confirmation, timed recovery wakes and voltage hysteresis.
- Reproducible CI builds, host tests and Heltec V4-only scope checks.

The firmware preserves MeshCore's existing `tx` meaning: the configured value is requested **SX1262 input power**, not guaranteed antenna output. The board may reduce the applied value according to the detected RF front end.

## Companion interface

The Heltec V4 Companion includes an on-device OLED control page so the screen can be disabled without a computer or CLI:

1. Short-press **PRG** to reach `OLED: ON`.
2. Hold PRG for about one second.
3. Release when prompted to save the persistent OFF state.
4. Hold PRG again for about one second to restore the display.

Bluetooth and LoRa remain active while the OLED is disabled. Incoming messages do not wake a persistently disabled screen, and the setting survives a reboot.

With the default OLED Companion configuration, the active six-digit Bluetooth PIN is generated at boot and displayed on the screen. The displayed PIN—not a presumed static code—must be used for first pairing.

## Solar Repeater behavior

The `heltec_v4_solar_repeater` target adds an unattended battery-protection profile:

- Three consecutive readings at or below **3.50 V** trigger protective recovery sleep.
- Identity and configuration writes are blocked before shutdown.
- OLED, GPS, SX1262 and the external RF front end are placed in low-power states.
- Recovery wakes use a timer at approximately 60-second intervals.
- Normal boot resumes only after the battery reaches **3.65 V**.

This policy is intentionally limited to the Solar Repeater. Companion targets retain continuous connectivity behavior instead of entering the solar recovery latch.

## Validation and releases

Every retained target is compiled by CI, and the repository runs native policy tests plus an automated audit that prevents unrelated board support from being reintroduced.

Initial physical validation on a Heltec V4.3 Companion has confirmed successful boot, Bluetooth pairing with the PIN shown on the OLED, on-device persistent display shutdown and restoration, and normal startup after removal from USB power. This does not yet constitute a full battery-runtime, RF-output or solar-deployment validation.

Validation releases remain prereleases until physical testing covers prolonged Bluetooth reconnection, current consumption, RF output, configuration persistence, full discharge behavior and solar recovery on the final hardware installation.

Complete ESP32-S3 images are published with the exact suffix **`-merged.bin`**. That suffix must be preserved when using the MeshCore Custom Firmware flasher so the complete image is written at address `0x0`. Application-only update images use the suffix **`-ota.bin`**.

## Upstream and contribution policy

Changes in this fork should be directly useful to supported Heltec V4 OLED hardware. General MeshCore protocol improvements should normally be proposed upstream first, then synchronized here with an audit of their impact on ESP32 power, RadioLib, display, GPS, storage and the Heltec RF front end.

Original MeshCore authors and contributors retain attribution for upstream work. MeshCore is distributed under the MIT License.

## Recommended GitHub About description

> Heltec V4-only MeshCore firmware for reliable Companion and Repeater operation, persistent OLED/GPS power control, calibrated battery reporting, protected storage and solar recovery.

## Suggested repository topics

`meshcore`, `heltec`, `heltec-v4`, `esp32-s3`, `lora`, `mesh-network`, `companion`, `repeater`, `solar-node`, `firmware`, `platformio`, `oled`, `gps`, `battery-management`


## Energy optimization generation

The current generation adds role-specific peripheral builds, cached battery
telemetry with fresh safety readings, cooperative ESP32 idle, optional DFS and
automatic light sleep, BLE/Wi-Fi/ESP-NOW power policies, runtime RX profiles and
a deep-sleep Sensor target. New installations request 11 dBm SX1262 input while
the detected-FEM policy retains the 22 dBm estimated antenna-output ceiling.
