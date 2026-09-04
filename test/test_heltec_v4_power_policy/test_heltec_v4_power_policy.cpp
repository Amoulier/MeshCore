#include <gtest/gtest.h>

#include "../../variants/heltec_v4/HeltecV4PowerPolicy.h"

using namespace heltec_v4;

TEST(HeltecV4CriticalBattery, FreshBootRejectsImplausibleAdcReadings)
{
  EXPECT_FALSE(shouldUseCriticalBatteryRecovery(0, false, 2500, 3500, 3650));
  EXPECT_FALSE(shouldUseCriticalBatteryRecovery(2499, false, 2500, 3500, 3650));
}

TEST(HeltecV4CriticalBattery, FreshBootSleepsAtOrBelowCutoff)
{
  EXPECT_TRUE(shouldUseCriticalBatteryRecovery(2500, false, 2500, 3500, 3650));
  EXPECT_TRUE(shouldUseCriticalBatteryRecovery(3500, false, 2500, 3500, 3650));
  EXPECT_FALSE(shouldUseCriticalBatteryRecovery(3501, false, 2500, 3500, 3650));
}

TEST(HeltecV4CriticalBattery, LatchedRecoveryUsesHysteresis)
{
  EXPECT_TRUE(shouldUseCriticalBatteryRecovery(0, true, 2500, 3500, 3650));
  EXPECT_TRUE(shouldUseCriticalBatteryRecovery(3649, true, 2500, 3500, 3650));
  EXPECT_FALSE(shouldUseCriticalBatteryRecovery(3650, true, 2500, 3500, 3650));
}

TEST(HeltecV4CriticalBattery, RuntimeCounterRequiresConsecutiveReadings)
{
  uint8_t count = 0;
  count = updateLowReadingCounter(count, 3500, 2500, 3500, 3);
  EXPECT_EQ(1, count);
  count = updateLowReadingCounter(count, 3490, 2500, 3500, 3);
  EXPECT_EQ(2, count);
  count = updateLowReadingCounter(count, 3510, 2500, 3500, 3);
  EXPECT_EQ(0, count);
  count = updateLowReadingCounter(count, 3400, 2500, 3500, 3);
  count = updateLowReadingCounter(count, 3400, 2500, 3500, 3);
  count = updateLowReadingCounter(count, 3400, 2500, 3500, 3);
  EXPECT_EQ(3, count);
}

TEST(HeltecV4BatteryPercent, CurvesHonorEndpointsAndMidpoints)
{
  EXPECT_EQ(100, batteryPercent(4300, PowerProfile::Standard));
  EXPECT_EQ(0, batteryPercent(3100, PowerProfile::Standard));
  EXPECT_EQ(0, batteryPercent(3500, PowerProfile::SolarRepeater));
  EXPECT_EQ(50, batteryPercent(3825, PowerProfile::Standard));
  EXPECT_EQ(50, batteryPercent(3825, PowerProfile::SolarRepeater));
  EXPECT_EQ(95, batteryPercent(4230, PowerProfile::Standard));
}

TEST(HeltecV4BatteryPercent, SlewRejectsShortTransmitSags)
{
  EXPECT_EQ(80, slewBatteryPercent(80, 45, 59999, 60000));
  EXPECT_EQ(79, slewBatteryPercent(80, 45, 60000, 60000));
  EXPECT_EQ(75, slewBatteryPercent(80, 45, 300000, 60000));
  EXPECT_EQ(81, slewBatteryPercent(80, 90, 60000, 60000));
  EXPECT_EQ(90, slewBatteryPercent(80, 90, 600000, 60000));
}

TEST(HeltecV4RadioPower, MapsDesiredAntennaPowerToSafeSx1262Input)
{
  EXPECT_EQ(11, gc1109RadioInputPower(22));
  EXPECT_EQ(9, kct8103lRadioInputPower(22));
  EXPECT_EQ(-9, gc1109RadioInputPower(0));
  EXPECT_EQ(-9, kct8103lRadioInputPower(0));
  EXPECT_EQ(21, gc1109RadioInputPower(28));
  EXPECT_EQ(21, kct8103lRadioInputPower(28));
}

TEST(HeltecV4RadioPower, PreservesLegacyRadioInputWhenAlreadySafe)
{
  EXPECT_EQ(10, clampGc1109RadioInput(10, 22));
  EXPECT_EQ(9, clampKct8103lRadioInput(10, 22));
  EXPECT_EQ(-9, clampGc1109RadioInput(-12, 22));
  EXPECT_EQ(-5, clampKct8103lRadioInput(-5, 22));
}

TEST(HeltecV4RadioPower, CapsLegacyRadioInputByDetectedFem)
{
  EXPECT_EQ(11, clampGc1109RadioInput(22, 22));
  EXPECT_EQ(9, clampKct8103lRadioInput(22, 22));
  EXPECT_EQ(22, gc1109EstimatedOutput(11));
  EXPECT_EQ(22, kct8103lEstimatedOutput(9));
  EXPECT_EQ(21, gc1109EstimatedOutput(10));
  EXPECT_EQ(22, kct8103lEstimatedOutput(9));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
