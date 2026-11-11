Stable MeshCore fork release for the Heltec WiFi LoRa 32 V4 OLED family.

## Changes

- **Optional GPS duty cycling.** Set acquisition, rest and fix-hold times from
  the Companion settings or CLI. Cycling is off by default. Use
  `get gps.cycle` and `gps diag` to inspect the receiver state.
- **Storage protection at startup.** The firmware initializes storage only
  after verifying the entire partition is erased. If mounting fails with
  existing data, startup pauses and reports a storage error. Solar Repeater
  continues monitoring for critical battery voltage in this state.
- **Receive and transmit handling during sleep.** Pending receive work,
  active transmissions and incoming Companion frames keep the MCU awake
  while they are being handled. The receive-preserving LoRa sleep path
  continues to use the existing radio power policy.
- **Path validation.** Checks plaintext length before decoding fields in
  a received `PATH` packet.
- **Display and diagnostics.** Companion shows uptime on the OLED. Repeater
  shows battery voltage and power-saving status. CLI commands
  `get power.reset`, `get sensor.pins` and `get radio.cad` expose reset
  reason, compiled sensor pins and CAD activity.

## Firmware and installation

The ten roles are Solar Repeater, Expansion Kit Repeater, ESP-NOW Repeater
Bridge, Room Server, Terminal Chat, Companion USB, Companion BLE,
Companion Wi-Fi, Sensor and KISS Modem.

Each role has an `-ota.bin` application image for compatible updates and a
`-merged.bin` image for fresh installations or recovery. The ZIP contains
all twenty images and the installation guide. Follow `INSTALLATION.txt`
for the correct image and flash address, and verify the download with
`SHA256SUMS.txt`.

## Validation

The firmware source passed 212 local host tests and compiled for all ten
roles. On the Companion BLE validation build, the Heltec V4.3 started
after a 20-second interruption of both USB and battery power with its
settings already saved. The node retained its identity and LoRa and GPS
settings. The phone reconnected, and the user confirmed their usual contacts
and channels were visible. The release packages are built from the tagged
source by GitHub Actions.

This independent fork carries forward the v1.1.2 fork base at `62e184a6`.
Its Heltec V4 specialization began from upstream MeshCore commit
`65aa1138ae0506703cab74d5e94bb924bd4516b4`.
