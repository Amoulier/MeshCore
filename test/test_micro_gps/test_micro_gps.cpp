#include <gtest/gtest.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>
#include <memory>
#include <string>

// Required native vtable boundary; the concrete production provider overrides it.
void LocationProvider::sendSentence(const char*) {}

namespace {

class BufferedUart : public Stream {
  std::string _input;
  size_t _position = 0;
public:
  unsigned reads = 0, available_calls = 0, writes = 0;
  int available() override { ++available_calls; return static_cast<int>(_input.size() - _position); }
  int read() override { ++reads; return _position < _input.size() ? _input[_position++] : -1; }
  int peek() override { return _position < _input.size() ? _input[_position] : -1; }
  size_t write(uint8_t) override { ++writes; return 1; }
  void feed(const std::string& input) { _input += input; }
};

class CapturedClock : public mesh::RTCClock {
public:
  unsigned updates = 0;
  uint32_t value = 0;
  uint32_t getCurrentTime() override { return value; }
  void setCurrentTime(uint32_t epoch) override { ++updates; value = epoch; }
};

std::string sentence(const std::string& body) {
  const std::string start = "$" + body;
  char checksum[3] = {};
  MicroNMEA::generateChecksum(start.c_str(), checksum);
  return start + "*" + checksum + "\r\n";
}

std::string rmc(const char* date = "091126", bool valid = true) {
  return sentence(std::string("GPRMC,000000.000,") + (valid ? "A" : "V") +
                  ",4807.038,N,01131.000,E,0.0,0.0," + date + ",,,A");
}

std::string gga(bool valid = true) {
  return sentence(std::string("GPGGA,000000.000,4807.038,N,01131.000,E,") +
                  (valid ? "1" : "0") + ",08,0.9,545.4,M,46.9,M,,");
}

class MicroGps : public ::testing::Test {
protected:
  BufferedUart uart;
  CapturedClock clock;
  RefCountedDigitalPin rail{36, LOW};
  std::unique_ptr<MicroNMEALocationProvider> provider;

  void SetUp() override {
    setMockTimeMs(0);
    memset(g_mock_gpio, 0, sizeof(g_mock_gpio));
    g_mock_pin_modes = g_mock_gpio_writes = 0;
    rail.begin();
    provider.reset(new MicroNMEALocationProvider(uart, &clock, 42, 34, &rail));
    // Production's cold-off path consumes initial-probe suppression without a
    // rail claim. Subsequent explicit starts therefore exercise the real path.
    provider->stop();
  }
  void parseAt(uint64_t now, const std::string& input) {
    setMockTimeMs(now);
    uart.feed(input);
    provider->loop();
  }
  void tickAt(uint64_t now) { setMockTimeMs(now); provider->loop(); }
  void start() { provider->begin(); provider->reset(); provider->syncTime(); }
};

TEST_F(MicroGps, RmcAndGgaAdvanceFixSequenceOnceEachIncludingCrLfTermination) {
  start();
  parseAt(1000, rmc());
  EXPECT_TRUE(provider->isValid());
  EXPECT_EQ(1U, provider->getFixSequence());
  parseAt(2000, gga());
  EXPECT_EQ(2U, provider->getFixSequence());
  parseAt(3000, rmc());
  EXPECT_EQ(3U, provider->getFixSequence());
}

TEST_F(MicroGps, NonPositionSentencesAndCachedReadsDoNotCreateNewFixes) {
  start();
  parseAt(1000, rmc());
  ASSERT_EQ(1U, provider->getFixSequence());
  parseAt(2000, sentence("GPGSV,1,1,04,01,40,083,41,02,17,273,35,03,13,172,32,04,09,304,30"));
  EXPECT_EQ(1U, provider->getFixSequence());
  for (uint32_t now : {3000U, 4000U, 5000U}) {
    tickAt(now);
    EXPECT_TRUE(provider->isValid());
    EXPECT_EQ(1U, provider->getFixSequence());
  }
}

TEST_F(MicroGps, BadChecksumAndInvalidPositionSentencesCannotRefreshFixSequenceOrAge) {
  start();
  parseAt(1000, rmc());
  std::string bad = rmc();
  const size_t checksum = bad.find('*') + 1;
  bad[checksum] = bad[checksum] == '0' ? 'F' : '0';
  parseAt(2000, bad);
  EXPECT_EQ(1U, provider->getFixSequence());
  parseAt(3000, rmc("091126", false));
  EXPECT_FALSE(provider->isValid());
  EXPECT_EQ(1U, provider->getFixSequence());
  parseAt(4000, gga(false));
  EXPECT_EQ(1U, provider->getFixSequence());
  uint32_t age;
  ASSERT_TRUE(provider->getFixAgeSeconds(age));
  EXPECT_EQ(3U, age);
}

TEST_F(MicroGps, FixAgeAndCacheSurvivePowerOffWithoutStatusReadsOperatingUart) {
  start();
  parseAt(1000, rmc());
  const long latitude = provider->getLatitude();
  provider->stop();
  const unsigned reads = uart.reads, available = uart.available_calls, writes = uart.writes;
  const unsigned gpio_writes = g_mock_gpio_writes;
  setMockTimeMs(6100);
  uint32_t age;
  ASSERT_TRUE(provider->getFixAgeSeconds(age));
  EXPECT_EQ(5U, age);
  EXPECT_EQ(1U, provider->getFixSequence());
  EXPECT_EQ(latitude, provider->getLatitude());
  EXPECT_FALSE(provider->isEnabled());
  char diagnostic[160];
  provider->formatDiagnostics(diagnostic, sizeof(diagnostic));
  EXPECT_EQ(reads, uart.reads);
  EXPECT_EQ(available, uart.available_calls);
  EXPECT_EQ(writes, uart.writes);
  EXPECT_EQ(gpio_writes, g_mock_gpio_writes);
  EXPECT_EQ(HIGH, digitalRead(36));
}

TEST_F(MicroGps, Esp32FixAgeUsesThe64BitBootTimerAcrossAnEntireMillisWrap) {
  start();
  parseAt(1000, rmc());
  provider->stop();
  const uint64_t elapsed = (uint64_t{1} << 32) + 5000ULL;
  setMockTimeMs(1000 + elapsed);
  uint32_t age;
  ASSERT_TRUE(provider->getFixAgeSeconds(age));
  EXPECT_EQ(static_cast<uint32_t>(elapsed / 1000ULL), age);
  EXPECT_EQ(1U, provider->getFixSequence());
}

TEST_F(MicroGps, RepeatedClaimReleaseDoesNotConsumeAnotherPeripheralsRailClaim) {
  rail.claim(); // An OLED or another peripheral owns the shared rail.
  provider->begin();
  provider->begin();
  EXPECT_EQ(LOW, digitalRead(36));
  provider->stop();
  provider->stop();
  EXPECT_EQ(LOW, digitalRead(36));
  rail.release();
  EXPECT_EQ(HIGH, digitalRead(36));
  provider->begin();
  EXPECT_EQ(LOW, digitalRead(36));
  provider->stop();
  EXPECT_EQ(HIGH, digitalRead(36));
}

TEST_F(MicroGps, InitialProbeSuppressionDoesNotClaimRailOrDelayBeforeExplicitEnable) {
  MicroNMEALocationProvider cold(uart, &clock, 42, 34, &rail);
  const uint32_t before = millis();
  cold.begin();
  cold.reset();
  EXPECT_EQ(before, millis());
  EXPECT_EQ(HIGH, digitalRead(36));
  EXPECT_FALSE(cold.isEnabled());
  EXPECT_EQ(0U, uart.reads);
  cold.stop();
  cold.begin();
  EXPECT_EQ(LOW, digitalRead(36));
  EXPECT_TRUE(cold.isEnabled());
  cold.stop();
}

TEST_F(MicroGps, GgaWithoutRmcDateNeverSetsTheRtcEvenAfterTheFixHold) {
  start();
  parseAt(1000, gga());
  for (uint32_t now : {2001U, 3002U, 4003U, 5004U, 6005U}) tickAt(now);
  EXPECT_TRUE(provider->isValid());
  EXPECT_EQ(0, provider->getTimestamp());
  EXPECT_EQ(0U, clock.updates);
  EXPECT_TRUE(provider->waitingTimeSync());
}

TEST_F(MicroGps, ExplicitSyncClearsOldParserDateAndRestartsTheValidFixCounter) {
  start();
  parseAt(1000, rmc());
  tickAt(2001); tickAt(3002); tickAt(4003);
  ASSERT_EQ(1U, clock.updates);
  provider->syncTime();
  EXPECT_FALSE(provider->isValid());
  EXPECT_EQ(0, provider->getTimestamp());
  EXPECT_TRUE(provider->waitingTimeSync());
  EXPECT_EQ(1U, provider->getFixSequence());
  parseAt(5004, rmc());
  EXPECT_EQ(1U, clock.updates);
  tickAt(6005); tickAt(7006);
  EXPECT_EQ(1U, clock.updates);
  tickAt(8007);
  EXPECT_EQ(2U, clock.updates);
}

TEST_F(MicroGps, InvalidCalendarDoesNotSetTheClockAndAValidFutureDateHasNo2030Cutoff) {
  start();
  parseAt(1000, rmc("310226"));
  tickAt(2001); tickAt(3002); tickAt(4003);
  EXPECT_EQ(0, provider->getTimestamp());
  EXPECT_EQ(0U, clock.updates);
  provider->syncTime();
  parseAt(5004, rmc("010131"));
  tickAt(6005); tickAt(7006); tickAt(8007);
  ASSERT_EQ(1U, clock.updates);
  EXPECT_EQ(1924992000U, clock.value); // 2031-01-01T00:00:00Z
}

TEST_F(MicroGps, EpochAboveSigned32BitRangeReachesTheUnsignedRtcInterface) {
  start();
  parseAt(1000, rmc("010140"));
  tickAt(2001); tickAt(3002); tickAt(4003);
  ASSERT_EQ(1U, clock.updates);
  EXPECT_EQ(2208988800U, clock.value); // 2040-01-01T00:00:00Z
  EXPECT_EQ(2208988800U, static_cast<uint32_t>(provider->getTimestamp()));
  // MinGW/ESP32 long32 exercises the real negative legacy return; Linux long64
  // exercises the same unsigned RTC boundary without pretending long is32.
  if (sizeof(long) == 4) EXPECT_LT(provider->getTimestamp(), 0);
}

TEST_F(MicroGps, RtcServiceDeadlineCrossesMillisRolloverWithExplicit32BitArithmetic) {
  start();
  const uint64_t begin = (uint64_t{1} << 32) - 2000;
  // Establish a legitimate pre-wrap next_check deadline with ordinary ticks;
  // jumping from0 by more than2^31 would violate signed-deadline assumptions.
  tickAt(0x40000000U);
  tickAt(0x80000000U);
  tickAt(0xc0000000U);
  parseAt(begin, rmc());
  tickAt(begin + 1001); tickAt(begin + 2002); tickAt(begin + 3003);
  EXPECT_EQ(1U, clock.updates);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
