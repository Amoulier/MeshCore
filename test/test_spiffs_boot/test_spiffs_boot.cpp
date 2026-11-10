#include <gtest/gtest.h>

#include <Arduino.h>
#include <SPIFFS.h>
#include <helpers/SafeSpiffs.h>
#include <helpers/ui/DisplayDriver.h>

namespace {
bool writes_allowed = true;
unsigned storage_failure_idle_calls = 0;

class SpiffsBoot : public testing::Test {
protected:
  void SetUp() override {
    SPIFFS = MockSpiffs{};
    Serial = MockStorageSerial{};
    g_storage_partition = esp_partition_t{1537, false};
    g_storage_partition_exists = true;
    g_storage_find_calls = 0;
    g_storage_found_type = -1;
    g_storage_found_subtype = -1;
    g_storage_lookup_label_null = false;
    g_storage_flash.assign(g_storage_partition.size, 0xff);
    g_storage_reads.clear();
    g_storage_read_failure = -1;
    g_storage_delay_calls = 0;
    g_storage_delay_ms = 0;
    writes_allowed = true;
    storage_failure_idle_calls = 0;
  }

  void expectNoAutomaticFormat() {
    for (bool flag : SPIFFS.format_on_fail_flags) EXPECT_FALSE(flag);
  }

  void expectPreservedFailure() {
    const auto original = g_storage_flash;
    EXPECT_FALSE(meshcoreMountSpiffs());
    EXPECT_EQ(SPIFFS.format_calls, 0u);
    EXPECT_EQ(g_storage_flash, original);
    ASSERT_EQ(SPIFFS.format_on_fail_flags.size(), 1u);
    expectNoAutomaticFormat();
  }
};

class StorageErrorDisplay : public DisplayDriver {
public:
  StorageErrorDisplay() : DisplayDriver(128, 64) {}
  bool on = false;
  unsigned frames = 0;
  std::vector<std::string> text;
  bool isOn() override { return on; }
  void turnOn() override { on = true; }
  void turnOff() override { on = false; }
  void clear() override {}
  void startFrame(ColorVal) override {}
  void setTextSize(int) override {}
  void setColor(ColorVal) override {}
  void setCursor(int, int) override {}
  void print(const char* value) override { text.emplace_back(value); }
  void fillRect(int, int, int, int) override {}
  void drawRect(int, int, int, int) override {}
  void drawXbm(int, int, const uint8_t*, int, int) override {}
  uint16_t getTextWidth(const char* value) override { return std::strlen(value) * 6; }
  void endFrame() override { ++frames; }
};
} // namespace

bool meshcorePersistentWritesAllowed() { return writes_allowed; }
void meshcoreStorageFailureIdle() { ++storage_failure_idle_calls; }

TEST_F(SpiffsBoot, ExistingFilesystemMountsWithoutReadingOrFormattingPartition) {
  SPIFFS.mount_results = {true};
  g_storage_flash[0] = 0x73;
  const auto original = g_storage_flash;
  EXPECT_TRUE(meshcoreMountSpiffs());
  EXPECT_EQ(g_storage_find_calls, 0u);
  EXPECT_TRUE(g_storage_reads.empty());
  EXPECT_EQ(SPIFFS.format_calls, 0u);
  EXPECT_EQ(g_storage_flash, original);
  expectNoAutomaticFormat();
}

TEST_F(SpiffsBoot, OnlyCompletelyErasedPartitionMayBeInitialized) {
  EXPECT_TRUE(meshcoreMountSpiffs());
  EXPECT_EQ(g_storage_found_type, ESP_PARTITION_TYPE_DATA);
  EXPECT_EQ(g_storage_found_subtype, ESP_PARTITION_SUBTYPE_DATA_SPIFFS);
  EXPECT_TRUE(g_storage_lookup_label_null);
  ASSERT_EQ(g_storage_reads.size(), 4u);
  EXPECT_EQ(g_storage_reads[0], std::make_pair(size_t{0}, size_t{512}));
  EXPECT_EQ(g_storage_reads[1], std::make_pair(size_t{512}, size_t{512}));
  EXPECT_EQ(g_storage_reads[2], std::make_pair(size_t{1024}, size_t{512}));
  EXPECT_EQ(g_storage_reads[3], std::make_pair(size_t{1536}, size_t{1}));
  EXPECT_EQ(SPIFFS.format_calls, 1u);
  ASSERT_EQ(SPIFFS.format_on_fail_flags.size(), 2u);
  expectNoAutomaticFormat();
}

TEST_F(SpiffsBoot, MissingPartitionCannotBeFormatted) {
  g_storage_partition_exists = false;
  expectPreservedFailure();
  EXPECT_TRUE(g_storage_reads.empty());
}

TEST_F(SpiffsBoot, ZeroSizedPartitionCannotBeInitialized) {
  g_storage_partition.size = 0;
  g_storage_flash.clear();
  expectPreservedFailure();
  EXPECT_TRUE(g_storage_reads.empty());
}

TEST_F(SpiffsBoot, EncryptedPartitionCannotBeDeclaredErased) {
  g_storage_partition.encrypted = true;
  expectPreservedFailure();
  EXPECT_TRUE(g_storage_reads.empty());
}

TEST_F(SpiffsBoot, ExistingDataAtFirstByteSurvivesMountFailure) {
  g_storage_flash.front() = 0;
  expectPreservedFailure();
}

TEST_F(SpiffsBoot, ExistingDataAtFinalByteSurvivesMountFailure) {
  g_storage_flash.back() = 0;
  expectPreservedFailure();
  ASSERT_EQ(g_storage_reads.size(), 4u);
}

TEST_F(SpiffsBoot, PartiallyInitializedFilesystemIsNeverReformatted) {
  const char marker[] = "identity-main-and-radio-preferences";
  std::memcpy(g_storage_flash.data() + 1000, marker, sizeof(marker));
  expectPreservedFailure();
}

TEST_F(SpiffsBoot, FirstReadErrorCannotBeTreatedAsErasedFlash) {
  g_storage_read_failure = 1;
  expectPreservedFailure();
}

TEST_F(SpiffsBoot, FinalReadErrorCannotBeTreatedAsErasedFlash) {
  g_storage_read_failure = 4;
  expectPreservedFailure();
  ASSERT_EQ(g_storage_reads.size(), 4u);
}

TEST_F(SpiffsBoot, CriticalBatteryGuardPreventsEvenBlankPartitionFormatting) {
  writes_allowed = false;
  expectPreservedFailure();
}

TEST_F(SpiffsBoot, FormatFailureDoesNotContinueToNormalStartup) {
  SPIFFS.format_result = false;
  EXPECT_FALSE(meshcoreMountSpiffs());
  EXPECT_EQ(SPIFFS.format_calls, 1u);
  ASSERT_EQ(SPIFFS.format_on_fail_flags.size(), 1u);
  expectNoAutomaticFormat();
}

TEST_F(SpiffsBoot, RemountFailureDoesNotTriggerAnotherFormat) {
  SPIFFS.mount_results = {false, false};
  EXPECT_FALSE(meshcoreMountSpiffs());
  EXPECT_EQ(SPIFFS.format_calls, 1u);
  ASSERT_EQ(SPIFFS.format_on_fail_flags.size(), 2u);
  expectNoAutomaticFormat();
}

TEST_F(SpiffsBoot, PreservedFilesystemMountsEvenWhenWritesAreForbidden) {
  SPIFFS.mount_results = {true};
  writes_allowed = false;
  EXPECT_TRUE(meshcoreMountSpiffs());
  EXPECT_EQ(SPIFFS.format_calls, 0u);
  EXPECT_TRUE(g_storage_reads.empty());
}

TEST_F(SpiffsBoot, ErrorHaltReportsFailureAndYieldsWithoutFormatting) {
  const auto original = g_storage_flash;
  EXPECT_THROW(meshcoreHaltForStorageFailure(), MockStorageHalt);
  EXPECT_EQ(g_storage_delay_calls, 1u);
  EXPECT_EQ(storage_failure_idle_calls, 1u);
  EXPECT_GT(g_storage_delay_ms, 0u);
  EXPECT_FALSE(Serial.messages.empty());
  EXPECT_EQ(SPIFFS.format_calls, 0u);
  EXPECT_EQ(g_storage_flash, original);
}

TEST_F(SpiffsBoot, ErrorHaltCanShowDiagnosticOnAvailableDisplay) {
  StorageErrorDisplay display;
  EXPECT_THROW(meshcoreHaltForStorageFailure(&display), MockStorageHalt);
  EXPECT_TRUE(display.on);
  EXPECT_FALSE(display.text.empty());
  EXPECT_EQ(display.frames, 1u);
  EXPECT_EQ(storage_failure_idle_calls, 1u);
  EXPECT_EQ(SPIFFS.format_calls, 0u);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
