# Heltec V4 high-power: over-air light-sleep validation

Bench test, 23 October 2026. Receiver: Heltec V4.3 OLED with KCT8103L,
running the high-power BLE light-sleep evaluation firmware. Sender: Seeed
XIAO nRF52 running official MeshCore `v1.18.0-81dbae5` via its existing USB
Companion interface. Both nodes already had identical saved modulation:
910.525 MHz, 62.5 kHz bandwidth, SF7, CR5.

The diagnostic image uses the same sleep implementation as the normal BLE
target, with USB diagnostics and `HELTEC_V4_ALLOW_USB_SLEEP_TEST` enabled.
Bluetooth is suspended, OLED is off, Wi-Fi/GPS are disabled, and the SX1262
and FEM stay in continuous RX with the existing range profile and external
LNA enabled. The normal image inhibits sleep while computer USB is active;
the test image overrides that guard for a bounded two-minute window and
automatically reconnects USB afterward.

Each 14-byte raw payload includes a test marker, a fresh 32-bit run ID and
a sequence number. Packets use a direct route with zero hops, avoiding
mesh flooding, contact changes and human messages. The sender transmits one
packet every two seconds. A RAM bitmap on the receiver records all unique
sequences without writing to the sleeping USB peripheral. Completed radio
TX counters confirm actual transmissions, rather than queue acceptance.

| Phase | Completed XIAO TX | Unique Heltec RX | Missing | MCU sleep time | GPIO wakes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Awake baseline before | 24 | 24 | 0 | 0 ms | 0 |
| Sleep window A | 48 | 48 | 0 | 113,419 ms | 12 |
| Sleep window B | 48 | 48 | 0 | 113,509 ms | 17 |
| Awake baseline after | 24 | 24 | 0 | 0 ms | 0 |

The two sleep windows accumulated 11,293 successful sleeps, 29 GPIO wakes,
11,264 timer wakes and 226,928 ms asleep within 240 seconds of test windows.
No PRG action was requested and no synthetic IRQ was armed during these tests.
GPIO wakes were recorded during actual over-air radio traffic, extending
the earlier synthetic SX1262 timeout-IRQ test. Sleep errors, radio receive
errors and core error flags remained zero. All 96 numbered packets sent
with sleep enabled arrived; the two awake baselines also received all 48.

The reverse-direction test completed another 24 Heltec transmissions, all
received by the XIAO with their expected run ID and sequence. The Heltec
reported 22 dBm requested, 21 dBm applied SX1262 input and 28 dBm estimated
FEM output before and after TX. No TX reduction was made. The XIAO's saved
radio settings and primary channel already matched the Heltec; no XIAO
firmware or configuration change was needed. Its configuration stays matched,
as requested. Both public identities were unchanged after the radio tests.

This is a short, close-range bench test. Observed Heltec RSSI ranged from
-38 to -5 dBm. It does not establish range, sensitivity, reliability under
other traffic patterns or guaranteed absence of future packet loss.
Current draw, battery runtime and conducted RF power still require external
measurement. The nominal 28 dBm setting is a FEM-table estimate, not an RF
power-meter measurement.

The RAM collector and USB sleep override are excluded from normal BLE,
USB and Wi-Fi Companion images; all three were rebuilt and their application
and merged image headers checked. All 83 host tests passed locally. The
previous ten-target build validation remains recorded separately. No passing
remote CI result was available for this validation; these checks ran locally.

After testing, the original normal BLE application slot was restored from the
verified 16 MiB pre-test snapshot. Esptool verified its on-device hash. BLE
advertising, a connection, uncached MeshCore service discovery and both GATT
characteristics passed afterward. The temporary diagnostic firmware is no
longer installed; persistent storage was not erased.

Full flash backups and detailed diagnostic logs remain local under ignored
`out/`; backups contain node secrets and must not be published.
