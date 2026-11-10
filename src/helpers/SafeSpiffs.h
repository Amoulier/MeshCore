#pragma once

#if defined(ESP32)
class DisplayDriver;

// Existing data must never be formatted because a mount failed. First-use
// initialization is allowed only after verifying the entire partition is erased.
bool meshcoreMountSpiffs();

// Optional board hook: keep battery protection alive without touching mesh or
// radio objects whose initialization has not run yet.
void meshcoreStorageFailureIdle();

// Does not return to setup/loop, where an unavailable filesystem could otherwise
// be mistaken for missing preferences or identity files.
[[noreturn]] void meshcoreHaltForStorageFailure(DisplayDriver* display = nullptr);
#endif
