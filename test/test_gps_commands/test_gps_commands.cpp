#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <helpers/sensors/GpsCommands.h>

// The production base declares this virtual without a default implementation.
// Supply the unused native boundary so the diagnostic provider has RTTI/vtable.
void LocationProvider::sendSentence(const char*) {}

namespace {

using gps::GpsCyclePrefs;
using gps::GpsDutyCycleConfig;

class ReplayStream : public Stream {
  std::string _text;
  size_t _position = 0;
public:
  explicit ReplayStream(const char* text) : _text(text) {}
  int available() override { return static_cast<int>(_text.size() - _position); }
  int read() override { return _position < _text.size() ? _text[_position++] : -1; }
  int peek() override { return _position < _text.size() ? _text[_position] : -1; }
};

class CaptureStream : public Stream {
  std::string _text;
  size_t number(unsigned long long value) {
    char buffer[24];
    const int count = snprintf(buffer, sizeof(buffer), "%llu", value);
    return write(reinterpret_cast<const uint8_t*>(buffer), count);
  }
public:
  size_t write(uint8_t value) override { _text.push_back(static_cast<char>(value)); return 1; }
  size_t write(const uint8_t* buffer, size_t count) override {
    _text.append(reinterpret_cast<const char*>(buffer), count); return count;
  }
  size_t print(unsigned char value, int = DEC) override { return number(value); }
  size_t print(int value, int = DEC) override { return number(value); }
  size_t print(unsigned int value, int = DEC) override { return number(value); }
  size_t print(long value, int = DEC) override { return number(value); }
  size_t print(unsigned long value, int = DEC) override { return number(value); }
  size_t print(long long value, int = DEC) override { return number(value); }
  size_t print(unsigned long long value, int = DEC) override { return number(value); }
  const std::string& text() const { return _text; }
};

class DiagnosticProvider : public LocationProvider {
public:
  int begins = 0, stops = 0, coordinates_read = 0;
  long getLatitude() override { ++coordinates_read; return 123; }
  long getLongitude() override { ++coordinates_read; return 456; }
  long getAltitude() override { ++coordinates_read; return 789; }
  long satellitesCount() override { return 4; }
  bool isValid() override { return true; }
  long getTimestamp() override { return 0; }
  void begin() override { ++begins; }
  void stop() override { ++stops; }
  void reset() override {}
  void loop() override {}
  bool isEnabled() override { return false; }
  void formatDiagnostics(char* out, size_t size) override {
    snprintf(out, size, "en:0 ub:20 ok:2 bad:0 sat:4 fix:1");
  }
};

class FakeSensors : public SensorManager {
public:
  bool accept_toggle = true, accept_interval = true, accept_cycle = true, accept_sync = true;
  bool user_requested = false, cycle_enabled = false, provider_available = true;
  int toggle_calls = 0, interval_calls = 0, configure_calls = 0, sync_calls = 0, diagnostic_reads = 0;
  uint32_t runtime_interval = 1;
  GpsDutyCycleConfig runtime_config;
  DiagnosticProvider provider;

  bool setSettingValue(const char* key, const char* value) override {
    if (strcmp(key, "gps") == 0) {
      ++toggle_calls;
      if (!accept_toggle) return false;
      user_requested = strcmp(value, "1") == 0;
      return true;
    }
    if (strcmp(key, "gps_interval") == 0) {
      ++interval_calls;
      if (!accept_interval) return false;
      return gps::detail::parseGpsUnsigned(value, runtime_interval);
    }
    return false;
  }
  bool configureGpsCycle(const GpsDutyCycleConfig& config, bool enabled) override {
    ++configure_calls;
    gps::GpsDutyCyclePolicy validator;
    if (!accept_cycle || !validator.configure(config)) return false;
    runtime_config = config;
    cycle_enabled = enabled;
    return true;
  }
  bool requestGpsSync() override { ++sync_calls; return accept_sync; }
  void formatGpsCycleDiagnostics(char* out, size_t size) override {
    ++diagnostic_reads;
    snprintf(out, size, "req:%u awake:0 cycle:%u state:rest", user_requested ? 1U : 0U,
             cycle_enabled ? 1U : 0U);
  }
  LocationProvider* getLocationProvider() override { return provider_available ? &provider : nullptr; }
};

class GpsCommands : public ::testing::Test {
protected:
  FakeSensors sensors;
  GpsCyclePrefs prefs;
  uint8_t requested = 0;
  uint32_t interval = 0;
  bool changed = false;
  char reply[160] = {};

  bool run(const char* command) {
    return gps::handleGpsCommand(command, sensors, requested, interval, prefs, changed, reply, sizeof(reply));
  }
};

TEST_F(GpsCommands, AllStatusFormsAreReadOnlyAndNeverReadCoordinatesOrWakeGps) {
  for (const char* command : {"gps", "gps cycle", "get gps.cycle", "gps diag"}) {
    changed = true;
    ASSERT_TRUE(run(command));
    EXPECT_FALSE(changed);
    EXPECT_EQ(0, sensors.toggle_calls);
    EXPECT_EQ(0, sensors.configure_calls);
    EXPECT_EQ(0, sensors.sync_calls);
    EXPECT_EQ(0, sensors.provider.begins);
    EXPECT_EQ(0, sensors.provider.stops);
    EXPECT_EQ(0, sensors.provider.coordinates_read);
  }
  EXPECT_EQ(3, sensors.diagnostic_reads);
  EXPECT_EQ(0, strncmp(reply, "req:0 ", 6));
}

TEST_F(GpsCommands, DiagnosticReportsMissingProviderWithoutEnablingAnything) {
  sensors.provider_available = false;
  ASSERT_TRUE(run("gps diag"));
  EXPECT_EQ(0, strncmp(reply, "Error:", 6));
  EXPECT_EQ(0, sensors.toggle_calls);
  EXPECT_FALSE(changed);
}

TEST_F(GpsCommands, UserOnOffPersistsOnlySuccessfulRealChanges) {
  ASSERT_TRUE(run("gps on"));
  EXPECT_EQ(1, requested);
  EXPECT_TRUE(changed);
  ASSERT_TRUE(run("gps on"));
  EXPECT_FALSE(changed);
  ASSERT_TRUE(run("gps off"));
  EXPECT_EQ(0, requested);
  EXPECT_TRUE(changed);
  ASSERT_TRUE(run("gps off"));
  EXPECT_FALSE(changed);
}

TEST_F(GpsCommands, RejectedUserToggleDoesNotChangeTheSavedRequest) {
  sensors.accept_toggle = false;
  ASSERT_TRUE(run("gps on"));
  EXPECT_EQ(0, requested);
  EXPECT_FALSE(changed);
  EXPECT_FALSE(sensors.user_requested);
}

TEST_F(GpsCommands, UserOffDominatesAnExplicitSyncRequest) {
  ASSERT_TRUE(run("gps sync"));
  EXPECT_EQ(0, sensors.sync_calls);
  EXPECT_EQ(0, sensors.provider.begins);
  EXPECT_FALSE(changed);
  ASSERT_TRUE(run("gps on"));
  ASSERT_TRUE(run("gps sync"));
  EXPECT_EQ(1, sensors.sync_calls);
  EXPECT_FALSE(changed);
}

TEST_F(GpsCommands, RejectedSyncDoesNotChangeAnyPreference) {
  ASSERT_TRUE(run("gps on"));
  sensors.accept_sync = false;
  ASSERT_TRUE(run("gps sync"));
  EXPECT_EQ(0, strncmp(reply, "Error:", 6));
  EXPECT_EQ(1, requested);
  EXPECT_EQ(0, prefs.enabled);
  EXPECT_FALSE(changed);
}

TEST_F(GpsCommands, OptingIntoTheCycleDoesNotTurnOnAnOwnerDisabledGps) {
  ASSERT_TRUE(run("gps cycle on"));
  EXPECT_EQ(1, prefs.enabled);
  EXPECT_TRUE(sensors.cycle_enabled);
  EXPECT_TRUE(changed);
  EXPECT_EQ(0, requested);
  EXPECT_EQ(0, sensors.toggle_calls);
  EXPECT_EQ(0, sensors.provider.begins);
  ASSERT_TRUE(run("gps cycle on"));
  EXPECT_FALSE(changed);
  ASSERT_TRUE(run("gps cycle off"));
  EXPECT_TRUE(changed);
  EXPECT_EQ(0, prefs.enabled);
  EXPECT_EQ(0, requested);
}

TEST_F(GpsCommands, AcceptedPeriodChangesAreAppliedAndSavedTogether) {
  ASSERT_TRUE(run("gps acquire 300"));
  EXPECT_TRUE(changed);
  EXPECT_EQ(300U, prefs.acquire);
  EXPECT_EQ(300U, sensors.runtime_config.acquire_seconds);
  ASSERT_TRUE(run("gps rest 1800"));
  EXPECT_EQ(1800U, prefs.rest);
  ASSERT_TRUE(run("gps hold 10"));
  EXPECT_EQ(10U, prefs.hold);
  ASSERT_TRUE(run("gps hold 10"));
  EXPECT_FALSE(changed);
  EXPECT_EQ(0, prefs.enabled);
}

TEST_F(GpsCommands, StrictDecimalRejectsSignsWhitespaceSuffixesAndOverflowBeforeRuntime) {
  for (const char* command : {"gps acquire ", "gps acquire +5", "gps acquire -5", "gps acquire  5",
                             "gps acquire 5 ", "gps acquire 5x", "gps acquire 5.0", "gps rest 1e3",
                             "gps hold 0x1", "gps interval 4294967296", "gps interval 99999999999999999"}) {
    ASSERT_TRUE(run(command)) << command;
    EXPECT_EQ(0, strncmp(reply, "Error:", 6)) << command;
    EXPECT_FALSE(changed);
    EXPECT_EQ(120U, prefs.acquire);
    EXPECT_EQ(900U, prefs.rest);
    EXPECT_EQ(5U, prefs.hold);
  }
  EXPECT_EQ(0, sensors.configure_calls);
  EXPECT_EQ(0, sensors.interval_calls);
}

TEST_F(GpsCommands, BoundariesAreCheckedBeforeAnyRuntimeOrSavedMutation) {
  for (const char* command : {"gps acquire 4", "gps acquire 3601", "gps rest 0", "gps rest 86401",
                             "gps hold 61", "gps interval 86401"}) {
    ASSERT_TRUE(run(command));
    EXPECT_EQ(0, strncmp(reply, "Error:", 6));
    EXPECT_FALSE(changed);
  }
  EXPECT_EQ(0, sensors.configure_calls);
  EXPECT_EQ(0, sensors.interval_calls);
  ASSERT_TRUE(run("gps acquire 0005"));
  EXPECT_TRUE(changed);
  EXPECT_EQ(5U, prefs.acquire);
  ASSERT_TRUE(run("gps hold 6"));
  EXPECT_FALSE(changed);
  EXPECT_EQ(5U, prefs.hold);
}

TEST_F(GpsCommands, RuntimeConfigurationRejectionRollsBackTheWholeProposedPreference) {
  sensors.accept_cycle = false;
  for (const char* command : {"gps cycle on", "gps acquire 300", "gps rest 1800", "gps hold 10"}) {
    ASSERT_TRUE(run(command));
    EXPECT_FALSE(changed);
    EXPECT_EQ(0, prefs.enabled);
    EXPECT_EQ(120U, prefs.acquire);
    EXPECT_EQ(900U, prefs.rest);
    EXPECT_EQ(5U, prefs.hold);
    EXPECT_EQ(120U, sensors.runtime_config.acquire_seconds);
  }
}

TEST_F(GpsCommands, ZeroIntervalRemainsAStoredLegacyValueButRuntimeUsesOneSecond) {
  ASSERT_TRUE(run("gps interval 60"));
  EXPECT_EQ(60U, interval);
  EXPECT_EQ(60U, sensors.runtime_interval);
  ASSERT_TRUE(run("gps interval 0"));
  EXPECT_EQ(0U, interval);
  EXPECT_EQ(1U, sensors.runtime_interval);
  EXPECT_TRUE(changed);
  ASSERT_TRUE(run("gps interval 0"));
  EXPECT_FALSE(changed);
}

TEST_F(GpsCommands, RejectedIntervalKeepsItsSavedValueAndRuntimeValue) {
  interval = 30;
  sensors.runtime_interval = 30;
  sensors.accept_interval = false;
  ASSERT_TRUE(run("gps interval 60"));
  EXPECT_FALSE(changed);
  EXPECT_EQ(30U, interval);
  EXPECT_EQ(30U, sensors.runtime_interval);
}

TEST_F(GpsCommands, PrefixLookalikesNeverReachMutatorsAndLegacyLocationCommandsFallThrough) {
  for (const char* command : {"gps onXYZ", "gps offXYZ", "gps syncXYZ", "gps cycle onXYZ",
                             "gps diagXYZ", "gps advertXYZ", "gps setlocXYZ", "get gps.cycleXYZ"}) {
    ASSERT_TRUE(run(command));
    EXPECT_EQ(0, strncmp(reply, "Error:", 6));
    EXPECT_FALSE(changed);
  }
  EXPECT_EQ(0, sensors.toggle_calls);
  EXPECT_EQ(0, sensors.sync_calls);
  EXPECT_EQ(0, sensors.configure_calls);
  for (const char* command : {"gps advert", "gps advert share", "gps advert none", "gps setloc", "get power.reset"}) {
    EXPECT_FALSE(run(command));
  }
}

TEST_F(GpsCommands, DiagnosticsRemainBoundedAndDoNotRequireAnOutputBuffer) {
  char guarded[] = {'L', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'R'};
  for (const char* command : {"gps", "gps diag", "gps cycle", "get gps.cycle"}) {
    ASSERT_TRUE(gps::handleGpsCommand(command, sensors, requested, interval, prefs, changed,
                                      guarded + 1, 8));
    EXPECT_EQ('L', guarded[0]);
    EXPECT_EQ('R', guarded[9]);
    EXPECT_EQ('\0', guarded[8]);
    ASSERT_TRUE(gps::handleGpsCommand(command, sensors, requested, interval, prefs, changed, nullptr, 0));
  }
  EXPECT_EQ(0, sensors.toggle_calls);
  EXPECT_EQ(0, sensors.provider.begins);
  changed = true;
  EXPECT_FALSE(gps::handleGpsCommand(nullptr, sensors, requested, interval, prefs, changed, nullptr, 0));
  EXPECT_FALSE(changed);
}

class LegacyGpsFields : public ConfigSerializer {
protected:
  void structure() override { def("en", enabled); def("int", interval); }
public:
  uint8_t enabled = 0;
  uint32_t interval = 0;
};

class PersistedGpsFields : public ConfigSerializer {
protected:
  void structure() override { def("gps", legacy); def("gps_cycle", cycle); }
public:
  LegacyGpsFields legacy;
  GpsCyclePrefs cycle;
};

TEST(GpsCyclePrefs, MissingLegacyCycleObjectPreservesUserSettingsWithOptInDisabled) {
  PersistedGpsFields loaded;
  ReplayStream input("{name:\"legacy\",radio:{tx:22},gps:{en:1,int:120}}");
  ASSERT_TRUE(loaded.loadSerial(input));
  loaded.cycle.normalize();
  EXPECT_EQ(1, loaded.legacy.enabled);
  EXPECT_EQ(120U, loaded.legacy.interval);
  EXPECT_EQ(0, loaded.cycle.enabled);
  EXPECT_EQ(120U, loaded.cycle.acquire);
  EXPECT_EQ(900U, loaded.cycle.rest);
  EXPECT_EQ(5U, loaded.cycle.hold);
}

TEST(GpsCyclePrefs, ConfigSerializerRoundTripPreservesCycleAndUserEnabledSeparately) {
  PersistedGpsFields saved;
  saved.legacy.enabled = 0;
  saved.legacy.interval = 0;
  saved.cycle.enabled = 1;
  saved.cycle.acquire = 300;
  saved.cycle.rest = 1800;
  saved.cycle.hold = 10;
  CaptureStream output;
  ASSERT_TRUE(saved.saveSerial(output));
  PersistedGpsFields loaded;
  ReplayStream input(output.text().c_str());
  ASSERT_TRUE(loaded.loadSerial(input));
  loaded.cycle.normalize();
  EXPECT_EQ(0, loaded.legacy.enabled);
  EXPECT_EQ(0U, loaded.legacy.interval);
  EXPECT_EQ(1, loaded.cycle.enabled);
  EXPECT_EQ(300U, loaded.cycle.acquire);
  EXPECT_EQ(1800U, loaded.cycle.rest);
  EXPECT_EQ(10U, loaded.cycle.hold);
}

TEST(GpsCyclePrefs, InvalidStoredConfigurationsDisableTheOptInAndRestoreAllDefaults) {
  for (const char* document : {"{en:1,acquire:4,rest:900,hold:0}",
                               "{en:1,acquire:120,rest:0,hold:5}",
                               "{en:1,acquire:5,rest:900,hold:6}",
                               "{en:1,acquire:120,rest:900,hold:61}",
                               "{en:257,acquire:300,rest:1800,hold:10}",
                               "{en:1,acquire:4294967301,rest:900,hold:0}",
                               "{en:1,acquire:120x,rest:900,hold:5}",
                               "{en:1,acquire:120,rest:-1,hold:5}"}) {
    GpsCyclePrefs loaded;
    ReplayStream input(document);
    ASSERT_TRUE(loaded.loadSerial(input)) << document;
    loaded.normalize();
    EXPECT_EQ(0, loaded.enabled) << document;
    EXPECT_EQ(120U, loaded.acquire);
    EXPECT_EQ(900U, loaded.rest);
    EXPECT_EQ(5U, loaded.hold);
  }
}

TEST(GpsCyclePrefs, NormalizationAlsoRejectsInvalidDirectAssignments) {
  GpsCyclePrefs prefs;
  prefs.enabled = 2;
  prefs.acquire = 300;
  prefs.normalize();
  EXPECT_EQ(0, prefs.enabled);
  EXPECT_EQ(120U, prefs.acquire);
  prefs.enabled = 1;
  prefs.acquire = 5;
  prefs.rest = 1;
  prefs.hold = 0;
  prefs.normalize();
  EXPECT_EQ(1, prefs.enabled);
  EXPECT_EQ(5U, prefs.config().acquire_seconds);
}

TEST(GpsCyclePrefs, ApplyingPreferencesIsDelegatedAtomicallyWithoutChangingTheUserRequest) {
  GpsCyclePrefs prefs;
  FakeSensors sensors;
  prefs.enabled = 1;
  ASSERT_TRUE(prefs.apply(sensors));
  EXPECT_TRUE(sensors.cycle_enabled);
  EXPECT_FALSE(sensors.user_requested);
  sensors.accept_cycle = false;
  prefs.acquire = 300;
  EXPECT_FALSE(prefs.apply(sensors));
  EXPECT_EQ(120U, sensors.runtime_config.acquire_seconds);
  EXPECT_EQ(300U, prefs.acquire);
  EXPECT_EQ(0, sensors.toggle_calls);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
