# Contributing

This repository accepts changes that are directly relevant to the **Heltec WiFi LoRa 32 V4 OLED** family represented by `variants/heltec_v4`.

## Scope

Appropriate contributions include Heltec V4 battery, display, GPS, ESP32-S3 power, storage, USB/Bluetooth/Wi-Fi companion, RadioLib, GC1109, and KCT8103L improvements.

Changes that add support for another board, architecture, display family, or vendor are outside this fork's scope and should be proposed to [meshcore-dev/MeshCore](https://github.com/meshcore-dev/MeshCore) instead.

Shared-core changes are acceptable only when they are required by a supported Heltec V4 target and remain compatible with the upstream MeshCore protocol.

## Development workflow

1. Create a focused branch from `main`.
2. Keep hardware-specific behavior guarded or contained in `variants/heltec_v4` whenever practical.
3. Avoid changing radio defaults, protocol behavior, or regional limits without documented evidence and validation.
4. Build every affected environment and run the native tests.
5. Describe the hardware revision, firmware target, test procedure, and observed result in the pull request.

Run all supported firmware builds with:

```bash
./build.sh
```

Run the host tests with:

```bash
pio test -e native -e native_kiss_modem -vv
```

## Pull-request expectations

A pull request should state:

- Heltec board revision tested, such as V4.2 or V4.3.
- OLED or expansion-kit configuration used.
- Battery, power source, radio region, and antenna assumptions when relevant.
- Before-and-after behavior.
- Any effect on Bluetooth, USB, Wi-Fi, GPS, OLED, LoRa receive, LoRa transmit, storage, or deep sleep.
- The exact PlatformIO environments successfully compiled.

Do not include production passwords, Wi-Fi credentials, private keys, device identities, or location data.

## Upstream synchronization

Upstream MeshCore changes should be reviewed rather than merged blindly. In particular, audit changes to `src/helpers`, `examples`, `arch/esp32`, RadioLib wrappers, power handling, display code, GPS code, storage, and `variants/heltec_v4` before integrating them.