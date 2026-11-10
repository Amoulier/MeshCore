#include <gtest/gtest.h>
#include <helpers/sensors/EnvironmentSensorManager.h>
#include <Wire.h>
#include <string>

// The real provider contract declares this optional hook out of line. This
// boundary stub is not used by the manager tests; the fake overrides it too.
void LocationProvider::sendSentence(const char*) {}
static int persistent_guard_queries = 0;
bool meshcorePersistentWritesAllowed() { persistent_guard_queries++; return false; }

namespace {
class FakeLocation : public LocationProvider {
public:
  int begins = 0, stops = 0, resets = 0, loops = 0, syncs = 0, probes = 0, sentences = 0;
  bool enabled = false, rail_on = false, valid = false;
  uint32_t sequence = 0, fix_at = 0;
  long latitude = 0, longitude = 0, altitude = 0;
  void begin() override { begins++; enabled = rail_on = true; }
  void stop() override {
    stops++; enabled = rail_on = false; Serial1.end();
    // Cached validity and sequence deliberately survive power-off.
  }
  void reset() override { resets++; }
  void loop() override { loops++; }
  void syncTime() override { syncs++; LocationProvider::syncTime(); }
  void sendSentence(const char*) override { sentences++; }
  bool waitFor(const char*, uint32_t) override { probes++; return false; }
  bool isEnabled() override { return enabled; }
  bool isValid() override { return valid; }
  long getLatitude() override { return latitude; }
  long getLongitude() override { return longitude; }
  long getAltitude() override { return altitude; }
  long satellitesCount() override { return valid ? 6 : 0; }
  long getTimestamp() override { return 1700000000L; }
  uint32_t getFixSequence() const override { return sequence; }
  bool getFixAgeSeconds(uint32_t& seconds) const override {
    seconds = static_cast<uint32_t>(millis() - fix_at) / 1000U;
    return valid;
  }
  void fix(long lat, long lon, long alt) {
    latitude = lat; longitude = lon; altitude = alt;
    valid = true; sequence++; fix_at = millis();
  }
};

class GpsRuntime : public ::testing::Test {
protected:
  FakeLocation location;
  EnvironmentSensorManager sensors{location};
  void SetUp() override {
    g_mock_millis = 100; Serial1 = {}; Wire = {}; Wire1 = {};
    persistent_guard_queries = 0;
    ASSERT_TRUE(sensors.begin());
  }
  std::string setting(const char* name) {
    const char* value = sensors.getSettingByKey(name);
    return value ? value : "<absent>";
  }
  void configure(uint32_t hold = 0) {
    ASSERT_TRUE(sensors.configureGpsCycle(gps::GpsDutyCycleConfig(5, 2, hold), true));
  }
  void acquireThenRest() {
    configure();
    ASSERT_TRUE(sensors.setSettingValue("gps", "1"));
    g_mock_millis += 5000;
    sensors.loop();
    ASSERT_FALSE(sensors.isGpsReceiverOn());
    ASSERT_EQ("1", setting("gps"));
  }
};

TEST_F(GpsRuntime, BootGpsOffNeverOpensUartOrProbesAndAppliesColdPowerOff) {
  EXPECT_TRUE(sensors.isGPSDetected()); // Logical support, not a physical probe.
  EXPECT_EQ("0", setting("gps"));
  EXPECT_FALSE(sensors.isGpsReceiverOn());
  EXPECT_FALSE(location.enabled); EXPECT_FALSE(location.rail_on);
  EXPECT_EQ(0, Serial1.begin_calls); EXPECT_EQ(0, Serial1.available_calls);
  EXPECT_EQ(0, location.begins); EXPECT_EQ(0, location.resets);
  EXPECT_EQ(0, location.probes); EXPECT_EQ(0, location.sentences);
  EXPECT_EQ(1, location.stops);
  EXPECT_EQ(0, Wire.probes); EXPECT_EQ(0, Wire1.probes);
}

TEST_F(GpsRuntime, UserEnableStartsOnceAndRepeatedRequestsAreIdempotent) {
  EXPECT_TRUE(sensors.setSettingValue("gps", "1"));
  EXPECT_TRUE(sensors.setSettingValue("gps", "1"));
  sensors.loop(); sensors.loop();
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_TRUE(location.rail_on);
  EXPECT_TRUE(Serial1.running);
  EXPECT_EQ(1, Serial1.begin_calls); EXPECT_EQ(1, location.begins); EXPECT_EQ(1, location.resets);
  EXPECT_EQ("1", setting("gps"));
}

TEST_F(GpsRuntime, AcquisitionTimeoutStopsUartAndRailThenRestExpiresOnce) {
  acquireThenRest();
  EXPECT_FALSE(location.rail_on); EXPECT_FALSE(Serial1.running);
  EXPECT_EQ(2, location.stops); // Cold-off plus the acquisition stop.
  EXPECT_GE(Serial1.end_calls, 2);
  g_mock_millis += 1999; sensors.loop();
  EXPECT_FALSE(sensors.isGpsReceiverOn()); EXPECT_EQ(1, location.begins);
  g_mock_millis++; sensors.loop();
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_TRUE(location.rail_on);
  EXPECT_EQ(2, location.begins); EXPECT_EQ(2, Serial1.begin_calls);
}

TEST_F(GpsRuntime, UserOffDuringRestCannotBeUndoneByTelemetryOrTimers) {
  acquireThenRest();
  ASSERT_TRUE(sensors.setSettingValue("gps", "0"));
  const int begins = location.begins, stops = location.stops, loops = location.loops;
  CayenneLPP telemetry;
  sensors.querySensors(TELEM_PERM_LOCATION | TELEM_PERM_ENVIRONMENT, telemetry);
  g_mock_millis += 60000; sensors.loop();
  sensors.querySensors(TELEM_PERM_LOCATION, telemetry);
  EXPECT_EQ("0", setting("gps")); EXPECT_FALSE(sensors.isGpsReceiverOn());
  EXPECT_EQ(0, telemetry.gps_calls); EXPECT_EQ(begins, location.begins);
  EXPECT_EQ(stops, location.stops); EXPECT_EQ(loops, location.loops);
  EXPECT_FALSE(location.rail_on); EXPECT_FALSE(Serial1.running);
}

TEST_F(GpsRuntime, DisablingTheCycleDuringRestResumesContinuousGps) {
  acquireThenRest();
  ASSERT_TRUE(sensors.setSettingValue("gps_cycle", "0"));
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_TRUE(location.rail_on);
  EXPECT_EQ(2, location.begins);
  g_mock_millis += 60000; sensors.loop();
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_EQ(2, location.begins);
}

TEST_F(GpsRuntime, GlobalPowerSavingOffResumesWithoutOverridingTheUserRequest) {
  acquireThenRest();
  sensors.setPowerSavingEnabled(false);
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_EQ(2, location.begins);
  EXPECT_EQ("1", setting("gps")); EXPECT_EQ("1", setting("gps_cycle"));
  sensors.setPowerSavingEnabled(true);
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_EQ(2, location.begins);
  sensors.setSettingValue("gps", "0");
  sensors.setPowerSavingEnabled(false);
  EXPECT_FALSE(sensors.isGpsReceiverOn()); EXPECT_FALSE(location.rail_on);
  EXPECT_EQ(2, location.begins);
}

TEST_F(GpsRuntime, ANewFixIsRequiredAndCachedLocationSurvivesRestWithoutWaking) {
  configure(1);
  location.fix(1000000, 2000000, 3000); // Synthetic old fix before acquisition.
  ASSERT_TRUE(sensors.setSettingValue("gps", "1"));
  g_mock_millis += 2000; sensors.loop();
  ASSERT_TRUE(sensors.isGpsReceiverOn()); // Cached validity cannot complete acquisition.
  location.fix(4000000, 5000000, 6000);
  sensors.loop();
  g_mock_millis += 999; sensors.loop();
  EXPECT_TRUE(sensors.isGpsReceiverOn());
  g_mock_millis++; sensors.loop();
  ASSERT_FALSE(sensors.isGpsReceiverOn());
  EXPECT_DOUBLE_EQ(4, sensors.node_lat); EXPECT_DOUBLE_EQ(5, sensors.node_lon);
  EXPECT_DOUBLE_EQ(6, sensors.node_altitude);
  const int begins = location.begins, loops = location.loops;
  location.latitude = location.longitude = 9000000; location.altitude = 9000;
  g_mock_millis += 1000; sensors.loop();
  CayenneLPP telemetry;
  sensors.querySensors(TELEM_PERM_LOCATION, telemetry);
  EXPECT_EQ(1, telemetry.gps_calls);
  EXPECT_DOUBLE_EQ(4, telemetry.latitude); EXPECT_DOUBLE_EQ(5, telemetry.longitude);
  EXPECT_DOUBLE_EQ(6, telemetry.altitude);
  EXPECT_EQ(begins, location.begins); EXPECT_EQ(loops, location.loops);
  EXPECT_FALSE(location.rail_on);
}

TEST_F(GpsRuntime, RestWakeDoesNotReuseThePreviousCycleFix) {
  configure(); ASSERT_TRUE(sensors.setSettingValue("gps", "1"));
  location.fix(1000000, 2000000, 3000); sensors.loop();
  ASSERT_FALSE(sensors.isGpsReceiverOn());
  g_mock_millis += 2000; sensors.loop();
  ASSERT_TRUE(sensors.isGpsReceiverOn());
  g_mock_millis += 1000; sensors.loop();
  EXPECT_TRUE(sensors.isGpsReceiverOn());
  location.fix(2000000, 3000000, 4000); sensors.loop();
  EXPECT_FALSE(sensors.isGpsReceiverOn());
}

TEST_F(GpsRuntime, InvalidConfigurationIsAtomicAndDoesNotOperatePowerOrStorage) {
  configure(1); ASSERT_TRUE(sensors.setSettingValue("gps", "1"));
  const int begins = location.begins, stops = location.stops;
  EXPECT_FALSE(sensors.configureGpsCycle(gps::GpsDutyCycleConfig(4, 2, 0), false));
  EXPECT_FALSE(sensors.setSettingValue("gps_rest", "0"));
  EXPECT_FALSE(sensors.setSettingValue("gps_acquire", "5garbage"));
  EXPECT_FALSE(sensors.setSettingValue("gps_hold", "6"));
  EXPECT_FALSE(sensors.setSettingValue("gps", "true"));
  EXPECT_FALSE(sensors.setSettingValue("gps_cycle", "2"));
  EXPECT_EQ("5", setting("gps_acquire")); EXPECT_EQ("2", setting("gps_rest"));
  EXPECT_EQ("1", setting("gps_hold")); EXPECT_EQ("1", setting("gps_cycle"));
  EXPECT_EQ("1", setting("gps")); EXPECT_TRUE(sensors.isGpsReceiverOn());
  EXPECT_EQ(begins, location.begins); EXPECT_EQ(stops, location.stops);
  EXPECT_EQ(0, persistent_guard_queries);
}

TEST_F(GpsRuntime, ExplicitSyncIsRejectedOffButCanInterruptAuthorizedRest) {
  EXPECT_FALSE(sensors.requestGpsSync());
  EXPECT_EQ(0, location.syncs); EXPECT_EQ(0, location.begins);
  acquireThenRest();
  const int syncs = location.syncs;
  EXPECT_TRUE(sensors.requestGpsSync());
  EXPECT_TRUE(sensors.isGpsReceiverOn()); EXPECT_TRUE(location.rail_on);
  EXPECT_EQ(2, location.begins); EXPECT_GT(location.syncs, syncs);
  sensors.requestGpsSync();
  EXPECT_EQ(2, location.begins);
}

TEST_F(GpsRuntime, DiagnosticsSeparateUserRequestFromAwakeAndDoNotWakeRest) {
  acquireThenRest();
  char diagnostics[160] = {};
  const int begins = location.begins;
  sensors.formatGpsCycleDiagnostics(diagnostics, sizeof(diagnostics));
  const std::string text(diagnostics);
  EXPECT_NE(std::string::npos, text.find("req:1"));
  EXPECT_NE(std::string::npos, text.find("state:resting"));
  EXPECT_NE(std::string::npos, text.find("awake:0"));
  EXPECT_EQ(begins, location.begins); EXPECT_FALSE(location.rail_on);
  EXPECT_LT(text.size(), sizeof(diagnostics));
}
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
