# Release process

Releases from this fork contain firmware only for the **Heltec WiFi LoRa 32 V4 OLED** family.

## Required validation

Before publishing a release:

1. Run the native test environments:

   ```bash
   pio test -e native -e native_kiss_modem -vv
   ```

2. Build every supported Heltec V4 environment:

   ```bash
   ./build.sh
   ```

3. Review the GitHub Actions result for the release commit.
4. Verify at least the affected firmware profile on the applicable Heltec V4 hardware revision.
5. Record the upstream MeshCore commit used as the synchronization base.

## Release contents

A release may include binaries for the retained `heltec_v4_*` environments only. Do not publish artifacts for TFT, R8, other Heltec devices, or any unrelated architecture.

Use release notes to identify:

- Supported Heltec V4 revisions.
- Included firmware targets.
- Hardware-tested targets.
- Battery, display, GPS, LoRa, connectivity, and storage changes.
- Known limitations and migration requirements.
- The upstream MeshCore base commit.

A suitable tag format is:

```text
heltec-v4-vX.Y.Z
```

This project is an independent hardware-focused fork and releases must not be represented as official MeshCore builds.