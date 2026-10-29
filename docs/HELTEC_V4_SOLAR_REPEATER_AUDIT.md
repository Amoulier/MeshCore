# Heltec V4 Solar Repeater audit

Audit dates: 28–29 October 2026, America/La_Paz. Hardware scope: one Heltec
V4.3 OLED with KCT8103L as the device under test, plus a second Heltec running
official Companion `v1.18.0-81dbae5` as the radio reference. The reference
firmware was retained. Shared radio settings were 910.525 MHz, 62.5 kHz
bandwidth, SF7 and CR5. These are strong-signal bench tests.

The source base is [PR #17](https://github.com/Amoulier/MeshCore/pull/17),
at `152fb0cf`, including its BLE and live-radio-retune fixes. This audit adds
the changes below on `fix/heltec-v4-solar-repeater-audit`; it does not publish
a stable release or merge these changes into `main`.

## Findings and corrections

| Commit | Problem and resulting behavior |
| --- | --- |
| `573c8920` | Companion and Repeater serializers shared `/prefs.json`, so saving one role could discard the other's fields. Solar now writes `/repeater_prefs.json` with its own transactional temporary/recovery files. First use can inherit compatible fields from the Companion profile without overwriting that source. |
| `be8732e6` | Solar's long sleep path could stall scheduling, USB and display handling. Solar now uses bounded 20 ms light-sleep windows only when RX is armed, no work is pending, GPS/display are off and no USB host is active. PRG display restoration receives a fresh visibility interval. The Solar profile also selects the user's requested maximum FEM output policy. |
| `c1bd1e33` | Anonymous direct clock/owner/regions queries trusted a declared reply-path length without checking the received plaintext size. A shared production helper now validates the header, encoding and available path bytes before handlers copy them. Login and other dispatch paths retain their behavior. Seven native tests cover truncation, hash widths, reserved encodings, zero-hop paths, padding and login boundaries. |
| `0f2fa19e` | Added a command-end USB drain. Its first 120-query test passed, but later tests still obtained partial replies; this change alone was insufficient. |
| `d709f9b7` | Adds bounded waiting for a writable USB FIFO and an additional packet flush at command completion, including commands that print directly. USB stress, powered forwarding, administration, display, persistence and the extended battery retest passed. The initial battery miss remains recorded below. |

The USB change addresses transfers that end on a full 64-byte packet. Its
extra termination flush follows the behavior documented in the FIFO HAL
and shown in [Espressif's current HWCDC implementation](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/HWCDC.cpp).
That source documents an extra empty-packet flush after a full packet; it
supports the correction's rationale, not a claim that this firmware uses
the current Arduino core. The project still builds with its pinned toolchain.

Application-only updates retained the existing filesystem and node identity.
No new device-flash backup or private-key export was made. The Companion
profile remains separate from Solar's saved preferences. Credentials,
private keys and node coordinates are excluded from this report and the
validation results; transient test contacts and permissions are restored.

The inherited Companion profile initially had forwarding disabled. Setup
explicitly enabled forwarding, kept power saving enabled and selected a
60-minute local advert interval; the CLI enforces that minimum for a configured
interval. These values survived reboot. GPS remained off and the RX profile
remained `range`, with the external LNA enabled.

Final readback confirmed that configuration and zero core/RX error flags.
The clock was aligned forward to host UTC after firmware installation and
reboots, then checked again without resetting the node. This was clock
maintenance, not a change to the saved role preferences. Normal inactivity
blanking wakes with a short PRG click; the persistent-off restoration gesture
and its 20-second visibility window were tested separately above.

On the high-power DUT, the selected policy is requested radio power 22 dBm,
applied SX1262 input 21 dBm and estimated FEM output 28 dBm. The 28 dBm value
comes from the board's mapping table; conducted RF output was not measured.
It is not a measured transmitter certification or range result.

## Source and build coverage

| Source | Validation completed |
| --- | --- |
| `c1bd1e33` | All 90 host tests passed, including the seven anonymous-request boundary tests. Normal Solar application and merged images compiled and were validated. |
| `0f2fa19e` | Normal Solar application and merged images compiled and were validated. Expansion Kit Repeater and Repeater Bridge ESP-NOW each also produced validated application and merged images, covering shared CLI/UI/anonymous-request code outside the Solar-specific guards. |
| `d709f9b7` | Normal Solar application and merged images validated. The application-only update was installed and its write hash verified. Final hardware coverage is recorded separately below. |

Image validation checks ESP32-S3 headers, embedded version, partition fit,
the exact application bytes inside the merged image at `0x10000`, hashes
and absence of Companion diagnostic hooks. These are normal firmware builds,
not builds with synthetic RX or USB-sleep test hooks enabled. Build manifests,
hashes and logs remain locally under `out/solar-repeater-audit-2026-10-28/`.
Build coverage is tied to the stated commits; the full matrix of all ten
firmware roles was not rebuilt for these Solar changes.

## Hardware results and retained failures

| Source/check | Observed result |
| --- | --- |
| `c1bd1e33` setup | Passed: role is Repeater, forwarding and power saving enabled, advert interval 60 minutes, matching reference radio settings, retained identity, and maximum-output policy configured. Settings were checked after reboot. |
| `0f2fa19e` USB | The first 120-query run passed. Subsequent runs without input purging and with tagged tracing failed on partial USB replies. The earlier pass is not treated as a complete USB qualification. |
| `0f2fa19e` direct forwarding | Passed ten unique payload circuits, duplicate suppression, wrong-next-hop filtering, forwarding-disabled filtering and restoration of forwarding. Returned payloads and packet/core counters were checked. |
| `0f2fa19e` remote administration | Native admin login and authenticated status passed. Temporary ACL permissions and the reference contact were restored. The raw overall result remains **false** because the final USB telemetry check timed out. Successful subchecks do not erase that failure. |
| `d709f9b7` USB | Passed 240/240 correlated queries without input purging, including two-second pauses every 20 queries. No timeout, retry or core error occurred. Final telemetry reported zero sleep entries and zero milliseconds asleep while USB was attached. |
| `d709f9b7` direct forwarding | Passed ten unique payload circuits, duplicate suppression, wrong-next-hop filtering, forwarding-disabled filtering and restored forwarding, including final telemetry. |
| `d709f9b7` remote administration | The complete suite passed: native admin login, authenticated status, temporary ACL/contact restoration and final USB telemetry without a timeout. |
| `d709f9b7` PRG/display | Passed with user confirmation: one approximately two-second PRG gesture restored the screen, which stayed visible for approximately 20 seconds and then turned off automatically. The display preference was restored and core errors stayed zero. |
| `d709f9b7` reboot persistence | Passed: identity, radio, forwarding, power saving, 60-minute adverts, TX policy and display preference matched before/after reboot. The active ACL also matched its baseline after temporary permission removal. Recovery-file contents were not examined or purged. |
| `d709f9b7` first battery forwarding run | **Failed overall: 9/10 matching returns**, with no return for circuit 3. MCU sleep was confirmed, but it does not turn that run into a pass. The failure was not reproduced in the subsequent 32-circuit samples. |
| `d709f9b7` extended USB control | Passed 32 unique forwarding circuits plus duplicate, wrong-next-hop, forwarding-disabled and restored-forwarding controls, including final telemetry. |
| `d709f9b7` extended battery retest | Passed 32/32 unique circuits: 32 exact payload returns and 32 corresponding raw-logger returns, with no late, missing or duplicate test payloads. Reference TX/RX and DUT direct RX/TX each increased by exactly 32. |
| `d709f9b7` DIRECT hash widths | Passed one exact-return circuit for each 1-, 2- and 3-byte hop-hash width. Packet ledgers, zero error flags, continuous uptime and unchanged preferences/power policy were checked. |
| `d709f9b7` FLOOD hash widths | Passed with the reference's signed self-adverts for 1-, 2- and 3-byte path hashes. Each signed payload returned unchanged with the exact DUT hop; an exact duplicate was suppressed. Settings stayed unchanged and core/RX errors remained zero. |

The first battery run recorded 6,023 sleeps, nine GPIO wakes, 6,014 timer
wakes and 120,625 ms asleep, with zero sleep/core errors and continuous
uptime. USB diagnostics recovered on the first attempt, and radio settings,
TX policy, forwarding, power saving and display preference were retained or
restored. The DUT packet delta was nine direct RX and nine direct TX, plus
one ambient flood RX and TX. Thus total RX/TX counters alone would obscure
the missing test circuit. The cause is not isolated to RF conditions or
firmware, and the failed record is retained separately.

The subsequent 32-circuit USB control and 32-circuit battery retest passed
on the same firmware. The improved battery ledger correlated both radios'
counters, exact payload notifications and raw-logger notifications, with
a separate late-arrival observation window. It recorded 9,918 sleeps,
30 GPIO wakes, 9,888 timer wakes and 198,458 ms asleep with zero sleep/core
errors. Uptime advanced continuously by 260 seconds, USB diagnostics
recovered on the first attempt, and settings were retained/restored. Those
samples did not reproduce the first miss; they do not establish its cause
or certify loss-free operation outside the tested conditions.

The DIRECT forwarding circuit uses two physical radios: the official Companion
sends a DIRECT RAW packet with the DUT's next-hop hash, the DUT retransmits
it and the same Companion receives the returned payload. This proves that
two-leg circuit and its controls; it is not a sender–repeater–receiver
three-node topology or a range test.

The FLOOD test also used these two controlled radios. Reference totals were
four transmissions and six receptions, including other observed paths;
those totals are not presented as a controlled three-node topology. Its
pass criteria required the exact DUT hop and unchanged signed payload,
with no raw advertisement contents or coordinates saved in the report.

Neither application's message FIFO was consumed. The remote status test uses
the native response notification, not a textual remote CLI reply. Textual
remote CLI response content remains unverified because the official Companion
queues those replies in the protected FIFO. A bad-password test was not
claimed: the Repeater can silently drop it, without a guaranteed failure push.

Detailed hardware records remain under
`out/repeater-hardware-validation-2026-10-28/`. Failed runs remain separate
from subsequent runs; no report replaces a failed result with a later pass.

## Remaining qualification

The planned short functional bench checks are complete on `d709f9b7`: USB,
powered and extended battery forwarding, DIRECT/FLOOD hash widths,
administration, PRG/display recovery and reboot persistence passed. The
first 9/10 battery result remains an unexplained observation despite the
later 32/32 samples; no causal fix or diagnosis is claimed for that miss.
These results do not certify all operating conditions. There is no
three-node or field forwarding validation, measured conducted RF output
or sensitivity, measured current or full battery runtime, photovoltaic
charging-cycle test, physical critical-battery recovery test,
or 48–72-hour stability run. No claim of measured autonomy or operation
through a real solar charging cycle follows from these bench checks.
