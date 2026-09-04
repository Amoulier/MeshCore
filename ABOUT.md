# About MeshCore for Heltec V4

**MeshCore for Heltec V4** is an independent, hardware-focused MeshCore firmware distribution for the **Heltec WiFi LoRa 32 V4 OLED** family.

The repository is intentionally limited to:

- Heltec V4.2 OLED hardware using the GC1109 radio front end.
- Heltec V4.3 OLED hardware using the KCT8103L radio front end.
- Heltec V4 expansion-kit configurations supported by the retained firmware targets.
- ESP32-S3, SX1262, SSD1306 OLED, GPS, Bluetooth, USB, Wi-Fi and ESP-NOW functionality required by those targets.

It does not support Heltec V4 TFT, Heltec V4 R8, other Heltec devices, or unrelated ESP32, nRF52, RP2040 and STM32 boards.

Shared MeshCore protocol, application, cryptography and test code remains only where it is required to compile or validate one of the supported Heltec V4 firmware environments.

The solar recovery path distinguishes a radio already placed into controlled sleep from an unknown cold-boot state. Only the unknown state holds SX1262 reset low; timer wakes preserve the lower-current controlled sleep path.

Validation tags package both application/OTA and complete merged images with checksums for every retained target. A validation prerelease remains explicitly pre-release until current-consumption, RF-output, connectivity and solar recovery tests have been completed on the physical Heltec V4 hardware.

This fork is maintained independently from `meshcore-dev/MeshCore` and must not be represented as an official MeshCore release.

## GitHub About description

> Heltec V4-only MeshCore firmware focused on reliable, power-aware OLED, GPS, battery and LoRa operation for the Heltec WiFi LoRa 32 V4/V4.3.

## Suggested repository topics

`meshcore`, `heltec`, `heltec-v4`, `esp32-s3`, `lora`, `mesh-network`, `firmware`, `platformio`, `oled`, `gps`, `battery-optimization`
