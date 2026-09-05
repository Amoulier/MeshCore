# Heltec V4 local OLED control validation

Use this checklist on the Companion BLE build after installation:

- Short PRG presses cycle through the home pages until **Display** appears.
- A long PRG press on the Display page turns the OLED off persistently.
- Incoming direct and channel messages do not wake the disabled OLED.
- Bluetooth remains connected or reconnectable while the OLED is disabled.
- LoRa reception and transmission continue while the OLED is disabled.
- A normal reboot leaves the OLED disabled.
- Holding PRG for about one second while the OLED is off restores it.
- Restoring the OLED does not toggle Bluetooth or alter the node identity.

This local control is intended to remove any dependency on a phone CLI or nearby computer.
