# Heltec V4 hardware validation gate

This checklist defines the physical tests required before the first stable release of this Heltec V4-only MeshCore distribution.

Automated compilation and native tests are necessary but do not validate RF output, current consumption, analog battery calibration, BLE behavior over time, or recovery with a real solar power system.

## Test records

For every test, record:

- board revision and FEM: V4.2/GC1109 or V4.3/KCT8103L;
- firmware tag and full commit SHA;
- battery, charger/controller, antenna or conducted RF setup;
- measured value, expected value, and pass/fail result;
- any reboot reason or serial log relevant to a failure.

Do not connect an RF power meter directly unless the instrument can safely accept the expected power. Use the required load and attenuation.

## Companion BLE

- [ ] A clean install retains the same MeshCore identity through at least three ordinary reboots.
- [ ] BLE is discoverable without pressing PRG.
- [ ] Initial connection succeeds.
- [ ] Reconnection succeeds after 5, 15, and 30 minutes of idle time.
- [ ] `set display off` cuts the OLED and persists after reboot.
- [ ] Direct messages and channel messages do not wake a persistently disabled OLED.
- [ ] Holding PRG for approximately one second restores and redraws the OLED.
- [ ] With GPS disabled, the expansion GPS is not powered, reset, or probed during boot.
- [ ] GPS can be enabled later without rebooting.
- [ ] A LoRa transmission does not create an abrupt published battery-percentage drop.
- [ ] Raw battery voltage still shows a real load-induced voltage sag.

## Standard Repeater

- [ ] Reception remains continuous with the OLED off.
- [ ] The TX indicator LED remains off.
- [ ] V4.2 reports GC1109 and V4.3 reports KCT8103L.
- [ ] V4.3 starts with its external LNA enabled.
- [ ] `set radio.fem.rxgain off` and `on` take effect and persist.
- [ ] No unwanted RF pulse, FEM lockup, or receiver loss occurs across boot, TX, RX, hibernate, and wake.
- [ ] Identity, preferences, contacts, channels, ACL, and region data survive induced power loss during controlled save tests.
- [ ] A valid OTA completes without the battery guard interrupting the update.

## Solar Repeater

- [ ] One or two transient samples at or below 3.50 V do not enter critical recovery.
- [ ] Three consecutive samples at or below 3.50 V enter critical recovery.
- [ ] A brief TX sag alone does not enter recovery.
- [ ] Recovery wakeups occur at approximately 60-second intervals.
- [ ] PRG and LoRa DIO1 do not escape timer-only recovery.
- [ ] OLED, GPS, SX1262, FEM, and their controllable rails remain in the intended low-power state between probes.
- [ ] The device does not complete a normal boot from 3.50 V through 3.649 V.
- [ ] A normal boot resumes at or above 3.65 V.
- [ ] Identity and persistent configuration remain unchanged after a complete low-battery/recovery cycle.
- [ ] Repeated discharge and recharge cycles work with the final panel, charger/controller, battery, and load.
- [ ] OTA is rejected below 3.65 V and remains uninterrupted once accepted above the threshold.

## Conducted RF power

Test both GC1109 and KCT8103L with several requested `tx` settings, including the maximum accepted value.

- [ ] Record the requested SX1262 power.
- [ ] Record the FEM-aware power actually applied by the firmware.
- [ ] Measure conducted output after the front end using suitable RF equipment.
- [ ] Confirm measured output does not exceed 22 dBm.
- [ ] Confirm the expected GC1109 and KCT8103L limits are selected.
- [ ] Confirm RX resumes correctly after every TX level tested.

## Current consumption

Record stable average current and relevant peaks for:

- [ ] Companion BLE with OLED on.
- [ ] Companion BLE with OLED persistently off.
- [ ] Standard Repeater in continuous RX.
- [ ] GPS enabled and disabled.
- [ ] Solar Repeater asleep between recovery probes.
- [ ] Solar Repeater during a recovery probe.
- [ ] Maximum permitted TX power.

## Stable-release criteria

A stable release is permitted only when:

1. V4.2/GC1109 and V4.3/KCT8103L both pass RF and power-state transitions;
2. prolonged BLE discovery and reconnection pass;
3. persistent identity remains byte-for-byte unchanged after the power-loss tests;
4. the 3.50 V cutoff and 3.65 V recovery behavior pass with the final solar system; and
5. conducted RF output remains at or below 22 dBm for all exposed settings.

Until then, generated binaries must remain marked as validation prereleases and must not be represented as production-ready.
