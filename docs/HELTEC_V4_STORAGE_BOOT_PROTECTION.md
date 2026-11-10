# Heltec V4 storage protection at startup

## Problem and scope

The six application entry points previously called `SPIFFS.begin(true)` and
ignored its return value. A mount failure could trigger automatic formatting;
continuing startup could also treat inaccessible identity/preferences as absent
and create defaults. This affected all ten retained firmware targets, including
Companion BLE and Solar Repeater.

Saved identity, preferences, contacts and channels normally survive complete
power removal because they are stored in flash. That does not make an interrupted
flash operation immune to corruption. Espressif describes this limitation in the
[ESP-IDF SPIFFS documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/storage/spiffs.html).

## Startup policy

Every ESP32 application now calls `meshcoreMountSpiffs()` before `radio_init()`
and before loading or creating identity, settings, interfaces or advertisements.

| Condition | Behavior |
| --- | --- |
| Existing filesystem mounts | Start normally using `SPIFFS.begin(false)`. No raw scan or formatting. |
| Mount fails; entire partition is erased (`0xFF`) | Initialize the new filesystem explicitly, subject to the persistent-write guard, then mount again without automatic formatting. |
| Mount fails; any byte contains data | Refuse formatting and stop normal startup. |
| Partition missing, zero-sized, encrypted or unreadable | Refuse formatting and stop normal startup. |
| Explicit first-use format or subsequent mount fails | Stop normal startup; do not retry formatting. |

The blank check reads the **whole** first DATA/SPIFFS partition, using the same
default selection as Arduino SPIFFS. Checking only the first sector would not
distinguish a blank device from one containing data elsewhere. Every read must
succeed. No automatic repair/check operation is requested on a damaged filesystem.
An interrupted first-use format can leave a nonblank, unmountable partition;
that state also requires deliberate recovery rather than another automatic erase.

On failure, startup stays in a diagnostic wait that yields to FreeRTOS. It never
returns to application `loop()`, creates a replacement identity, starts BLE/Wi-Fi,
or transmits advertisements. USB repeatedly reports the error. An available,
enabled display shows an error; a saved preference to disable the screen remains
respected. There is no automatic factory reset or normal CLI in this state.

Solar Repeater continues its existing critical-battery monitoring through a
board-specific error-wait hook. Its emergency shutdown uses GPIO holds/reset
without calling the not-yet-initialized radio or sensor APIs. Normal runtime
battery thresholds, sampling, sleep policy and shutdown callbacks are unchanged.

## Validation

Local software validation on 2026-11-09 passed all 212 host cases (196 existing
and 16 new), across seven native environments. All ten normal firmware targets
built successfully, including their twenty application/merged images. The six
entry-point guards and the Solar strong board hook were checked separately.

`native_spiffs_boot` compiles the actual `SafeSpiffs.cpp` against controlled
SPIFFS/partition/USB mocks. Its 16 regression cases cover:

- Existing filesystem startup without raw reads or formatting.
- Completely erased flash, including the final partial read.
- Existing data at the first and last byte and partially initialized storage.
- Missing, empty, encrypted and unreadable partitions.
- The persistent-write guard and format/remount failures.
- Diagnostic waiting with and without a display, and the board protection hook
  executing before yielding.

The suite is included in both the ordinary CI and stable-release validation.
Tests of this startup policy simulate failures; they are not physical power-cut
tests and do not prove recovery of an arbitrarily corrupted SPIFFS filesystem.

## Limits

This change prevents automatic erasure in response to a mount failure. It does
not recover data already corrupted, make flash writes physically atomic, or
change existing per-file recovery behavior when a filesystem mounts but a file
is damaged. Saved data and changes still pending in RAM are different: a power
cut can discard pending changes, queued messages, counters and the current time.

The existing temporary/backup-file protection remains in place. No storage
layout, settings schema, TX power, LoRa receive policy or normal power-saving
configuration changes are introduced. No device memory was read, exported,
modified or erased during this software validation.
