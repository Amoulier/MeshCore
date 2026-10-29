Stable MeshCore fork release for Heltec WiFi LoRa 32 V4 OLED.

Release preparation date: 29 October 2026.

This release carries forward the fork's display, PRG, GPS/accessory-power,
protected-storage and FEM-aware radio changes, and integrates the BLE and
Solar Repeater corrections found during the hardware audit.

## Changes

- **Companion BLE:** finishes controller shutdown after an asynchronous peer
  disconnect. Rebuilding BLE now keeps UART characteristic handles stable,
  so an existing bond can reconnect after Bluetooth is restored.
- **Live radio changes:** enters standby and invalidates the previous RX
  state before changing frequency/modulation, allowing continuous receive
  to be rearmed without a reboot.
- **Solar Repeater:** uses bounded 20 ms light-sleep windows while the radio
  stays in receive. Pending work, an active USB host, GPS or a visible display
  prevent this sleep path. PRG display restoration gets a fresh visibility
  interval, and USB replies ending on a full 64-byte packet are terminated
  correctly instead of leaving the host waiting for the rest of a reply.
- **Role changes:** Solar writes its own `/repeater_prefs.json` profile.
  Compatible settings can be inherited from the earlier shared profile
  without overwriting the Companion preferences left on the node.
- **Repeater request bounds:** rejects truncated anonymous reply paths before
  clock, owner or region handlers copy path bytes. Seven host tests cover
  the boundary cases.
- **Maximum-TX policy:** Companion BLE and Solar Repeater select the nominal
  28 dBm FEM policy on the tested high-power V4.3/KCT8103L board, using 21 dBm
  SX1262 input. These are mapping-table estimates, not measured RF output.
  Other roles keep their existing estimated 22 dBm ceiling. RX profiles
  continue to control reception independently.

## Firmware roles and files

The ten retained roles are Solar Repeater, Expansion Kit Repeater, ESP-NOW
Repeater Bridge, Room Server, Terminal Chat, Companion USB, Companion BLE,
Companion Wi-Fi, Sensor and KISS Modem. There is no separate standard
Repeater or low-power BLE image; the retained BLE image includes the
power-saving behavior.

Each role has two images:

- `*-ota.bin`: application image for a compatible update path; direct serial
  updates to the release's standard `app0` layout use `0x10000`.
- `*-merged.bin`: complete bootloader/partition/application image for a fresh
  installation or recovery, written at `0x0`.

The ZIP groups the twenty firmware images. Use the installation guide and
`SHA256SUMS.txt` supplied with this release, and retain the exact image suffix.
A full-device erase is a separate step and is not part of a routine update.

## Validation and scope

The physical bench results below apply to BLE validation builds at
`b6bf74e5` and Solar validation builds at `d709f9b7`. The stable release
images are newly compiled; their exact binary bytes have not been physically
tested on the nodes. The functional corrections from those tested sources
are carried into this release, while final source/build hashes and packaging
validation are recorded separately from those source-specific bench results.

The BLE changes passed 83 host tests and ten authenticated suspend/rebuild
cycles with stable GATT handles and an existing bond. The normal BLE
candidate also passed battery reception, sleep-counter, Windows reconnection
and user-confirmed phone reconnection checks.

The Solar audit passed 90 host tests at `c1bd1e33`. Its final `d709f9b7`
bench checks passed 240 correlated USB queries, native
administrator login and status, PRG/display restoration, reboot persistence,
and DIRECT/FLOOD forwarding with 1-, 2- and 3-byte path hashes. A 32-circuit
USB control and a 32-circuit battery retest both passed. The battery retest
matched all 32 payload returns and raw RX logs, with exactly 32 direct RX/TX
increments on both radios, no late or duplicate test returns and no
sleep/core/RX error flags.

The first ten-circuit battery run returned **9/10** and remains a failed
observation. The missing circuit did not appear in the DUT's direct RX/TX
counters. The later samples did not reproduce that miss; its cause was not
established, and no fix or loss-free guarantee is claimed for it.

Physical checks used a Heltec V4.3/KCT8103L and an official-firmware Heltec
Companion as a reference. The two-radio forwarding circuit is not a
sender–repeater–receiver three-node topology or a range test. Measured current,
full battery runtime, conducted RF output/sensitivity, a real photovoltaic
charging cycle, physical critical-battery recovery and a 48–72-hour soak
remain outside the completed qualification. Wi-Fi/ESP-NOW and other roles
are not covered by the Solar/BLE physical bench results.

Detailed source-specific coverage, retained failures and limits are recorded
in `docs/HELTEC_V4_BLE_RETUNE_FIX_VALIDATION.md` and
`docs/HELTEC_V4_SOLAR_REPEATER_AUDIT.md` at the release tag.
