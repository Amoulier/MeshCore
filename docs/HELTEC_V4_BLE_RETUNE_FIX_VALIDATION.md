# Heltec V4 BLE shutdown and live-retune fixes

Validation date: 28 October 2026. Scope: Companion BLE on a Heltec V4.3
with KCT8103L, using a second Heltec running official MeshCore as the radio
reference. Shared settings: 910.525 MHz, 62.5 kHz bandwidth, SF7 and CR5.
These are short-range, strong-signal bench results, not RF sensitivity or
range measurements.

## Failures reproduced in 1.1.1

**Asynchronous BLE shutdown.** The second repeated BLE cycle left the interface
disabled while the host and controller remained enabled: `bt:0`,
`suspended:0`, `controller:2`, `host:2`, with no reported BT error. Waiting
15 seconds did not finish shutdown. `disable()` initiates an asynchronous
peer disconnect, but disabled interfaces are skipped by
`MultiSerialInterface::checkRecvFrame()`. The intended shutdown retry was
only reached from that input-polling path.

The fix overrides `SerialBLEInterface::loop()` to keep servicing power
policy. `MultiSerialInterface::loop()` already calls every registered
interface, including disabled ones, so shutdown can finish after the
disconnect callback.

**Live radio retuning.** At unchanged bench spacing, reception was 5/5 before
switching from 910.525 MHz/SF7 to 869.618 MHz/SF8 and immediately back.
After that change it was 0/5, despite matching saved settings and zero core
error flags. Restarting restored 5/5. The pre-fix hot-retune observations
were recorded in the session console; the retained pre-fix JSON covers
reboot recovery, not a separate capture of the 0/5 run.

`CustomSX1262Wrapper::setParams()` previously programmed the modem without
leaving RX or invalidating the wrapper's RX state. The fix calls `idle()`
first: the chip enters standby and the wrapper becomes idle, allowing the
Dispatcher receive path to restart RX after configuration. RadioLib already
requests image calibration for the large frequency change; no missing
manual calibration is claimed. Configuration return values remain ignored
by the existing void API. Retuning during an active TX can interrupt that
packet; the controlled retune test used an empty outgoing queue.

The two fixes add five source lines, including comments, in
`SerialBLEInterface.h` and `CustomSX1262Wrapper.h`. They do not reduce TX
power or change the saved RX profile.

## Results with both fixes

Source: `20b21572` (following BLE shutdown fix `66db4e14`). Hardware tests
used diagnostic firmware `v1.1.1-D2-20b21572`.

| Check | Observed result |
| --- | --- |
| BLE shutdown/resume | 10/10 cycles passed connection and uncached UART GATT service/characteristic checks. |
| Heap after warm-up | Off-state median remained 96,200 bytes; on-state median remained 89,756 bytes. No observed decline across the measured cycles. |
| Timer wake | 472 sleeps, 472 timer wakes, 9,489 ms asleep; no sleep/core errors. |
| Synthetic radio IRQ | 472 sleeps, one GPIO wake, 471 timer wakes, 9,480 ms asleep; SPI error 0, timeout IRQ flag 512. This is not an RF packet test. |
| PRG wake | 2,055 sleeps, one GPIO wake, 2,054 timer wakes and 41,307 ms asleep; no sleep/core errors. Firmware reported the display on, and the user confirmed the screen lit. |
| Live retune without reboot | 5/5 baseline and 5/5 after the same frequency/SF round trip; uptime remained continuous and core errors stayed zero. |
| Private reception and queue retention | Six incoming private messages survived BLE shutdown/rebuild cycles; eight more arrived during a sleep window. All 14 were verified for content and order on the DUT. |
| Private ACKs | 20 matching ACKs: six reference-to-DUT, six DUT-to-reference and eight reference-to-DUT during sleep. |
| Private messages during sleep | 8/8 received and acknowledged; 111,173 ms asleep, 5,535 sleep entries, eight GPIO wakes and no sleep error. |
| Host tests | All 83 tests passed locally with the combined fixes. |
| Normal firmware build matrix | All ten targets and their 20 application/merged images compiled and validated from `20b21572`. |

The reference node's application message queue was never consumed. Its six
reverse-direction messages have radio ACK verification; full content/order
verification through the reference application remains pending. The diagnostic
suite's temporary contacts were removed or restored, and identity, radio
configuration and maximum-TX policy were preserved. The ten-cycle GATT test did not exercise
the authenticated Companion protocol; that was checked separately on the
normal candidate below.

A normal BLE candidate, `v1.1.2-rc-20b21572`, was built in application and
merged formats with diagnostic hooks excluded. Its application image was
installed and the on-device write hash verified. The computer authenticated
over BLE with MITM protection and successfully used the Companion protocol;
identity, radio configuration and maximum-TX policy were preserved. This is
not a published release.

## Normal candidate: battery-only results

The user confirmed disconnecting the DUT's USB and declared an 8,000 mAh
battery. Companion uptime advanced from 185 to 257 seconds without a reset.
Firmware-reported battery voltage changed from 4,323 mV with USB attached
to 4,206 mV after removal. These are voltage readings, not measured current,
verified battery capacity or an autonomy result.

Four private transmissions with the DUT awake on battery received four
matching ACKs. Another twelve sent after a 315-second disconnected wait also
received matching ACKs. The user confirmed that all 16 test messages appeared
in the MeshCore phone app without duplicates. Following recovery of the
computer connection, the user also confirmed that the phone app reopened
and showed normal data. The computer never read the DUT's message queue
during this battery-only test.

After PRG wake, a computer attempt to enumerate the full GATT tree returned
Windows `ERROR_BAD_COMMAND`. Recovery included renewing only the computer's
DUT bond and using uncached, UART-service-only discovery. A later connection
reused that bond and read the normal candidate's diagnostics successfully,
without resetting the node or erasing its data. A subsequent cleanup
connection also succeeded. The earlier failure with a previously paired
computer immediately after BLE off/on remains relevant; successful phone
operation alone does not resolve that paired-client problem.

The recovered normal-firmware counters established 4,875 sleeps, four GPIO
wakes, 4,871 timer wakes and 98,024 ms asleep, with sleep error zero. Uptime
was 1,683 seconds; packet counters were 17 received and 16 transmitted,
with zero receive/core errors. This confirms that the normal candidate
entered MCU sleep on battery without a diagnostic USB sleep override while
the radio tests continued. The exact shutdown instant was not sampled
directly; the transmission schedule used a 315-second disconnected wait.

Cleanup then removed both battery-test temporary contacts and verified both
identities and the shared radio settings. Maximum TX remained requested
22 dBm, applied SX1262 input 21 dBm and estimated output 28 dBm; RX remained
in the range profile with the LNA on. Display and sleep were enabled. At
cleanup, cumulative normal-firmware counters reached 12,050 sleeps, four
GPIO wakes, 12,046 timer wakes and 242,319 ms asleep with error zero;
uptime was 2,337 seconds and core errors remained zero.

## Follow-up: stable GATT characteristic handles

A source audit found that Arduino BLE's `BLECharacteristicMap` iterates a
map keyed by object pointers. Freeing and recreating characteristic objects
can change their ordering and therefore their assigned attribute handles.
Observed UART TX/RX handles changed from 43/41 to 41/44 after reconstruction.
This supplies a firmware-side mechanism for stale paired-client handle
mappings, so the computer failure must not be dismissed as solely a host
problem.

The follow-up in `b6bf74e5` gives TX/RX deterministic allocation order using
a private characteristic-pair object: TX is declared before RX in the same
allocation. On shutdown, the pair is destroyed before the service. The new
allocation path is guarded by `HELTEC_V4_COMPANION_LIGHT_SLEEP`; other roles
retain their existing path.

Diagnostic firmware `v1.1.1-D3-b6bf74e5` was compiled, installed as an
application-only update and its on-device write hash verified. All 83 host
tests passed for this source. The normal BLE candidate,
`v1.1.2-rc-b6bf74e5`, was also built in application and merged formats with
diagnostic hooks excluded. It has now been installed as an application-only
update with the write hash verified. A computer connection reused its
existing bond, confirmed the exact normal version, TX handle 41 and RX
handle 44, and successfully used the Companion protocol. Identity, shared
radio settings and maximum TX were preserved, with zero core errors. Its
battery-only regression is recorded below. Build coverage is recorded
separately: all ten targets/20 images at `20b21572`, and the new normal BLE
build at `b6bf74e5`. The follow-up allocation change is confined to the
BLE light-sleep flag.

All ten cycle records in the first D3 authenticated test passed. Each cycle
reused the existing computer bond, exercised the Companion protocol and
verified uncached UART-service attributes. No new pairing or unpairing was
performed during those cycles. TX handle 41, its CCCD handle 43 and RX
handle 44 stayed constant. Uptime was continuous and core errors remained
zero. The off-state heap median changed from 93,456 to 93,452 bytes; the
on-state median changed from 86,828 to 86,832 bytes. No material downward
trend was observed in this short sample.

That first run's raw overall result remains **false**: its final USB telemetry
snapshot timed out after the restoration commands. Its failure record is
retained. Later queries responded with sleep enabled, unchanged identity/
radio settings, uptime 91 seconds and zero core errors; those reads do not
retroactively replace the missing final snapshot in the first run.

A separate rerun completed ten new authenticated cycles and passed the
entire suite, including final-state capture. The final read succeeded on
the first attempt; no retry was needed. The existing bond was again reused
without pairing or unpairing, and TX/CCCD/RX handles remained 41/43/44. The
off-state heap median changed from 93,592 to 93,596 bytes, and the on-state
median from 86,928 to 86,952 bytes. Final telemetry confirmed sleep enabled,
zero BT/sleep/core errors and uptime 282 seconds. The original failed run
and this completed rerun remain separate records.

## Final normal candidate: battery regression

The normal `v1.1.2-rc-b6bf74e5` candidate passed a separate test with the
DUT's USB removed and the reference node still on battery. Both nodes kept
their identities and matching 910.525 MHz/BW62.5/SF7/CR5 settings. Two private
messages received matching ACKs while the DUT was awake. After disconnecting
the computer's BLE link and waiting 315 seconds for the configured automatic
shutdown, four more private messages received matching ACKs. The total
disconnected interval before requesting local BLE wake was 331 seconds;
the exact automatic shutdown instant was not directly sampled.

Following the user's PRG/BLE wake, the computer reconnected with the existing
bond, without pairing, unpairing or resetting the node. The complete NUS
table matched its initial layout: service 40, TX 41, CCCD 43 and RX 44.
Uptime remained continuous, advancing from 108 to 546 seconds. Sleep
counters increased by 2,715 entries, five GPIO wakes, 2,710 timer wakes and
54,511 ms asleep; sleep and core errors stayed zero. The GPIO count includes
the user's PRG action and is not attributed entirely to radio packets.

Maximum TX remained requested 22 dBm, applied radio input 21 dBm and
estimated output 28 dBm. Both temporary contacts were restored/removed and
verified, and display and sleep were enabled. Neither application message
queue was read by the computer. The six test messages remain for phone
synchronization. Matching radio ACKs verify reception; phone content/order
verification has not been performed in this run.
The user also confirmed that the phone app reconnected to this final build,
opened normally and showed the usual data without another pairing step.

## Remaining qualification

The D3 authenticated-cycle rerun, normal candidate's connected checks and
normal `b6bf74e5` battery regression and final phone reconnection are complete.
The 83 host tests cover the follow-up; the earlier
full matrix and the new BLE build have the distinct source coverage recorded
above. Phone delivery of sixteen messages was separately confirmed on the
earlier `20b21572` normal candidate.
Measured current, full battery runtime, conducted RF output, sensitivity and
48–72-hour stability have not been established by these results. The 28 dBm
setting remains a FEM-table estimate, not measured RF output. Other firmware
roles are outside this physical test scope.

At this point, `main` and the published `heltec-v4-energy-v1.1.1` tag remain
at `62a864c2`; the fixes are on the working branch. Detailed diagnostic logs
and backups remain local under ignored `out/` and are not published here.
