#include <gtest/gtest.h>
#include <helpers/sensors/TelemetrySubscription.h>

TEST(TelemetrySubscription, RejectsTruncatedAndOversizedThresholdRequests) {
  const uint8_t request[] = {1,0,0,0,60,0,0,4, 1,LPP_TEMPERATURE,0,10};
  EXPECT_TRUE(telemetry_subscription::validRequest(request, sizeof(request), 14));
  for (size_t length = 0; length < sizeof(request); ++length)
    EXPECT_FALSE(telemetry_subscription::validRequest(request, length, 14));
  EXPECT_FALSE(telemetry_subscription::validRequest(request, sizeof(request), 3));
}

TEST(TelemetrySubscription, RejectsInvalidChannelsTypesAndNegativeThresholds) {
  const uint8_t invalid[][4] = {{0,LPP_TEMPERATURE,0,1},
    {1,255,0,1}, {1,LPP_TEMPERATURE,255,255}, {1,LPP_GPS,0,1}};
  for (auto& record : invalid)
    EXPECT_FALSE(telemetry_subscription::validThresholds(record, sizeof(record)));
}

TEST(TelemetrySubscription, HumidityDoesNotConsumeFollowingChannel) {
  const uint8_t thresholds[] = {1,LPP_RELATIVE_HUMIDITY,4, 2,LPP_VOLTAGE,0,5};
  EXPECT_TRUE(telemetry_subscription::validThresholds(thresholds, sizeof(thresholds)));
  EXPECT_FLOAT_EQ(2, LPPData::getFloat(thresholds+2, 1, LPPData::getMultiplier(LPP_RELATIVE_HUMIDITY), false));
}

TEST(TelemetryEncoding, SignedFourByteValueRoundTrips) {
  uint8_t data[4];
  EXPECT_EQ(4, LPPData::putFloat(data, -100, 4, 1, true));
  EXPECT_FLOAT_EQ(-100, LPPData::getFloat(data, 4, 1, true));
}

TEST(TelemetryEncoding, TruncatedReadDoesNotChangeOutput) {
  const uint8_t truncated[] = {1,LPP_TEMPERATURE,0};
  LPPReader reader(truncated, sizeof(truncated));
  uint8_t channel, type;
  ASSERT_TRUE(reader.readHeader(channel, type));
  float value = 99;
  EXPECT_FALSE(reader.readTemperature(value));
  EXPECT_FLOAT_EQ(99, value);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
