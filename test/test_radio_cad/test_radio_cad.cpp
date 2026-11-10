#include <gtest/gtest.h>
#include <helpers/radiolib/RadioLibWrappers.h>

namespace {
class TestBoard : public mesh::MainBoard {
public:
  uint16_t getBattMilliVolts() override { return 4000; }
  const char* getManufacturerName() const override { return "test"; }
  void reboot() override {}
  uint8_t getStartupReason() const override { return BD_STARTUP_NORMAL; }
};
class TestWrapper : public RadioLibWrapper {
public:
  float current_rssi = -100;
  TestWrapper(PhysicalLayer& radio, TestBoard& board) : RadioLibWrapper(radio, board) {}
  bool isReceivingPacket() override { return false; }
  void setParams(float, float, uint8_t, uint8_t) override {}
  float getCurrentRSSI() override { return current_rssi; }
  void seedNearSaturation() { _cad_busy_count = UINT32_MAX - 1; }
};
class RadioCad : public ::testing::Test {
protected:
  PhysicalLayer physical;
  TestBoard board;
  TestWrapper wrapper{physical, board};
  void SetUp() override { wrapper.begin(); }
};

TEST_F(RadioCad, DisabledCadDoesNotScanOrClaimAnOperationalBusyMeasurement) {
  physical.scan_result = RADIOLIB_LORA_DETECTED;
  EXPECT_FALSE(wrapper.isCadEnabled());
  EXPECT_FALSE(wrapper.isChannelActive());
  EXPECT_EQ(0, physical.scans);
  EXPECT_EQ(0U, wrapper.getCadBusyCount());
}

TEST_F(RadioCad, OnlyRealLoraDetectionIncrementsAndEveryScanRearmsRx) {
  wrapper.setCADEnabled(true);
  physical.scan_result = RADIOLIB_CHANNEL_FREE;
  EXPECT_FALSE(wrapper.isChannelActive());
  EXPECT_EQ(0U, wrapper.getCadBusyCount());
  physical.scan_result = -123; // Driver error remains busy, but is not LoRa detection.
  EXPECT_TRUE(wrapper.isChannelActive());
  EXPECT_EQ(0U, wrapper.getCadBusyCount());
  physical.scan_result = RADIOLIB_LORA_DETECTED;
  EXPECT_TRUE(wrapper.isChannelActive());
  EXPECT_EQ(1U, wrapper.getCadBusyCount());
  EXPECT_EQ(3, physical.scans);
  EXPECT_EQ(3, physical.starts);
  EXPECT_EQ(0, physical.output_power_writes);
  EXPECT_EQ(0, physical.tx_starts);
  EXPECT_TRUE(wrapper.isInRecvMode());
}

TEST_F(RadioCad, RssiThresholdDoesNotCountAsHardwareCadDetection) {
  wrapper.setCADEnabled(true);
  wrapper.triggerNoiseFloorCalibrate(14);
  wrapper.current_rssi = 20;
  EXPECT_TRUE(wrapper.isChannelActive());
  EXPECT_EQ(0, physical.scans);
  EXPECT_EQ(0U, wrapper.getCadBusyCount());
}

TEST_F(RadioCad, CounterSaturatesAndExistingStatsResetStartsANewEpoch) {
  wrapper.setCADEnabled(true);
  wrapper.seedNearSaturation();
  physical.scan_result = RADIOLIB_LORA_DETECTED;
  wrapper.isChannelActive(); wrapper.isChannelActive();
  EXPECT_EQ(UINT32_MAX, wrapper.getCadBusyCount());
  wrapper.resetStats();
  EXPECT_EQ(0U, wrapper.getCadBusyCount());
  EXPECT_TRUE(wrapper.isCadEnabled());
  wrapper.isChannelActive();
  EXPECT_EQ(1U, wrapper.getCadBusyCount());
  wrapper.setCADEnabled(false);
  EXPECT_FALSE(wrapper.isChannelActive());
  EXPECT_EQ(1U, wrapper.getCadBusyCount()); // Disabled retains history, not a new scan.
}
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
