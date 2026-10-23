# About MeshCore for Heltec V4

**MeshCore for Heltec V4** is an independent, unofficial, hardware-focused MeshCore firmware distribution maintained exclusively for the **Heltec WiFi LoRa 32 V4 OLED** family.

This fork keeps the MeshCore protocol and the shared application code required by Heltec V4 firmware while concentrating development on reliable Companion, Repeater and Sensor operation. It is developed primarily on the **Heltec V4.3 OLED**, while retaining support for compatible V4.2 hardware.

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

1. Improve Heltec V4 power behavior without sacrificing LoRa transmission or normal role functionality.
2. Make Companion, Repeater and Sensor firmware dependable for daily and unattended use.
3. Protect node identity and configuration against low-voltage shutdowns and interrupted flash writes.
4. Provide hardware-specific controls directly on the node whenever relying on a computer or an unavailable mobile-app control would be impractical.

## Firmware roles

The maintained distribution contains ten Heltec V4 OLED targets; Solar Repeater replaces the official standard Repeater.

The repository builds Heltec V4 firmware for:

- Bluetooth, USB and Wi-Fi Companion nodes.
- Solar Repeater nodes with critical-battery recovery.
- Expansion-kit and ESP-NOW bridge Repeaters.
- Room Server, Terminal Chat, Sensor and USB KISS modem roles.

The complete environment list and build commands are documented in [`README.md`](README.md).

## Heltec V4-specific improvements

The maintained firmware adds or strengthens:

- Calibrated 12-bit battery sampling with averaged readings, Heltec-specific discharge curves and a stabilized displayed percentage.
- Persistent OLED shutdown, automatic-wake suppression and physical VEXT management where the rail is not required by another peripheral.
- GPS-disabled boot that avoids powering, resetting, probing or delaying for an unused GPS.
- TX and status LED suppression for lower unattended power use.
- Automatic GC1109/KCT8103L detection, KCT8103L receive-gain control and FEM-aware transmit limits.
- Full Bluetooth-host/controller shutdown after five disconnected minutes, followed by explicit ESP32-S3 light sleep while LoRa remains in continuous receive.
- DIO1, PRG and maintenance-timer wake paths for the BLE-off Companion state.
- Maximum physical TX locked across every Heltec V4 role at nominal 28 dBm for the high-power BLE Companion and the existing estimated 22 dBm ceiling in other roles.
- Safer identity and configuration persistence using temporary files, verified recovery copies and write blocking during critical battery recovery.
- Reproducible CI builds, host tests and Heltec V4-only scope checks.

The normal `tx` field remains present for MeshCore protocol compatibility, but this fork intentionally prevents a lower saved value or client command from reducing physical transmit power. The common maximum request is mapped by the detected front end. High-power BLE uses 21 dBm SX1262 input for nominal 28 dBm output; other roles retain 11 dBm on GC1109 and 9 dBm on KCT8103L for estimated 22 dBm output.

## Companion interface

The Heltec V4 Companion includes an on-device OLED control page so the screen can be disabled without a computer or CLI:

1. Short-press **PRG** to reach `OLED: ON`.
2. Hold PRG for about one second.
3. Release when prompted to save the persistent OFF state.
4. Hold PRG again for about one second to restore the display.

LoRa remains active while the OLED is disabled. Incoming messages do not wake a persistently disabled screen, and the setting survives a reboot.

With the default OLED Companion configuration, the active six-digit Bluetooth PIN is generated at boot and displayed on the screen. The displayed PIN—not a presumed static code—must be used for first pairing.

After five minutes without a BLE connection, the firmware disables Bluedroid and the ESP32-S3 Bluetooth controller after freeing GATT attributes. The MCU then enters explicit light sleep whenever the mesh is idle, while the SX1262 continues receiving and wakes the processor through DIO1. To restore Bluetooth, use the Bluetooth page and hold PRG; the node restores its encrypted GATT service without rebooting, preserving identity, contacts, channels, queued messages and saved settings.

## Validation and releases

Every retained target is compiled by CI, and the repository runs native policy tests plus an automated audit that prevents unrelated board support from being reintroduced.

Initial physical validation on a Heltec V4.3 Companion confirmed successful boot, Bluetooth pairing with the PIN shown on the OLED, on-device persistent display shutdown and restoration, and normal startup after removal from USB power. The later report of less than 24-hour runtime identified that stopping advertisements alone did not remove Bluetooth host/controller current; the current generation replaces that behavior with full BLE deinitialization and explicit BLE-off light sleep.

Physical qualification is still required for measured current draw, complete battery runtime, conducted RF output, sensitivity, prolonged reconnection, configuration persistence and full discharge behavior on the final hardware installation.

Complete ESP32-S3 images are published with the exact suffix **`-merged.bin`**. That suffix must be preserved when using the MeshCore Custom Firmware flasher so the complete image is written at address `0x0`. Application-only update images use the suffix **`-ota.bin`**.

## Upstream and contribution policy

Changes in this fork should be directly useful to supported Heltec V4 OLED hardware. General MeshCore protocol improvements should normally be proposed upstream first, then synchronized here with an audit of their impact on ESP32 power, RadioLib, display, GPS, storage and the Heltec RF front end.

Original MeshCore authors and contributors retain attribution for upstream work. MeshCore is distributed under the MIT License.

## Recommended GitHub About description

> Heltec V4-only MeshCore firmware with BLE-off light sleep, persistent OLED/GPS controls, calibrated battery reporting, protected storage and FEM-aware maximum TX.

## Suggested repository topics

`meshcore`, `heltec`, `heltec-v4`, `esp32-s3`, `lora`, `mesh-network`, `companion`, `repeater`, `firmware`, `platformio`, `oled`, `gps`, `battery-management`

## Energy optimization generation

Ten targets are retained, with Solar Repeater replacing the official standard Repeater. They share role-specific peripheral builds, cached battery telemetry, cooperative ESP32 idle, BLE/Wi-Fi power saving and runtime RX profiles. Companion BLE fully powers down the Bluetooth stack after five disconnected minutes and uses explicit light sleep while LoRa remains available. Every target requests the FEM-aware maximum, using nominal 28 dBm in high-power BLE and the existing estimated 22 dBm ceiling in other roles.