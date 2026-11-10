#include "SafeSpiffs.h"

#if defined(ESP32)
#include <Arduino.h>
#include <SPIFFS.h>
#include <esp_partition.h>

#include "PersistentWriteGuard.h"
#include "ui/DisplayDriver.h"

namespace {

bool spiffsPartitionIsErased()
{
  // SPIFFS.begin() with no label uses the first DATA/SPIFFS partition too.
  const esp_partition_t* partition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
  if (partition == nullptr || partition->size == 0 || partition->encrypted) {
    return false;
  }

  uint8_t buffer[512];
  for (size_t offset = 0; offset < partition->size;) {
    const size_t remaining = partition->size - offset;
    const size_t length = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
    if (esp_partition_read(partition, offset, buffer, length) != ESP_OK) {
      return false;
    }
    for (size_t i = 0; i < length; ++i) {
      if (buffer[i] != 0xFF) return false;
    }
    offset += length;
  }
  return true;
}

} // namespace

__attribute__((noinline)) void meshcoreStorageFailureIdle() __attribute__((weak));
__attribute__((noinline)) void meshcoreStorageFailureIdle() {}

bool meshcoreMountSpiffs()
{
  if (SPIFFS.begin(false)) return true;

  // A mount failure alone says nothing about whether valuable data exists.
  // Never auto-format or repair a nonempty/unreadable partition.
  if (!spiffsPartitionIsErased() || !meshcorePersistentWritesAllowed()) {
    return false;
  }
  if (!SPIFFS.format()) return false;
  return SPIFFS.begin(false);
}

[[noreturn]] void meshcoreHaltForStorageFailure(DisplayDriver* display)
{
  Serial.begin(115200);
  if (display != nullptr) {
    display->turnOn();
    display->startFrame(0);
    display->setTextSize(1);
    display->setColor(1);
    display->setCursor(0, 0);
    display->print("Storage error");
    display->setCursor(0, 16);
    display->print("Data preserved");
    display->setCursor(0, 32);
    display->print("Connect USB");
    display->endFrame();
  }

  // Yield to keep USB and the watchdog healthy, but never start the mesh,
  // advertise a replacement identity, or write defaults over inaccessible data.
  for (;;) {
    meshcoreStorageFailureIdle();
    Serial.println("Storage unavailable; automatic erase disabled. Node stopped to preserve data.");
    delay(1000);
  }
}
#endif
