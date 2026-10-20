#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "../../examples/companion_radio/NodePrefs.h"
#include "../../src/helpers/CommonRadioPrefs.cpp"

// Formatting boundary: run the real command parser without Arduino's ltoa.
const char* StrHelper::ftoa(float value) {
  static char buffer[32];
  snprintf(buffer, sizeof(buffer), "%g", static_cast<double>(value));
  return buffer;
}
const char* StrHelper::ftoa3(float value) { return ftoa(value); }

class ReplayStream : public Stream {
  const char* _text;
  int _pos = 0;
  int _len;

public:
  explicit ReplayStream(const char* text) : _text(text), _len(strlen(text)) { }

  int available() override { return _len - _pos; }
  int read() override { return _pos < _len ? _text[_pos++] : -1; }
  int peek() override { return _pos < _len ? _text[_pos] : -1; }
};

class CaptureStream : public Stream {
  std::string _text;

  size_t emit(long long value) {
    char text[24];
    int length = snprintf(text, sizeof(text), "%lld", value);
    return write(reinterpret_cast<const uint8_t*>(text), length);
  }

public:
  size_t write(uint8_t value) override {
    _text.push_back(static_cast<char>(value));
    return 1;
  }

  size_t write(const uint8_t* buffer, size_t size) override {
    _text.append(reinterpret_cast<const char*>(buffer), size);
    return size;
  }

  size_t print(unsigned char value, int = DEC) override { return emit(value); }
  size_t print(int value, int = DEC) override { return emit(value); }
  size_t print(unsigned int value, int = DEC) override { return emit(value); }
  size_t print(long value, int = DEC) override { return emit(value); }
  size_t print(unsigned long value, int = DEC) override { return emit(value); }
  size_t print(long long value, int = DEC) override { return emit(value); }
  size_t print(unsigned long long value, int = DEC) override { return emit(value); }

  const std::string& text() const { return _text; }
};

#if 0
// Re-enable test once we can SET fem_ values in companion
TEST(CompanionNodePrefs, RxGainSettingsRoundTripIndependently) {
  NodePrefs saved;
  saved.rx_boosted_gain = 0;
  saved.radio_fem_rxgain = 1;
  saved.radio_fem_txgain = 0;

  CaptureStream output;
  ASSERT_TRUE(saved.saveSerial(output));
  EXPECT_NE(std::string::npos, output.text().find("rxgain:0"));
  EXPECT_NE(std::string::npos, output.text().find("fem_rxgain:1"));
  EXPECT_NE(std::string::npos, output.text().find("fem_txgain:0"));

  ReplayStream input("{radio:{rxgain:1,fem_rxgain:0,fem_txgain:1}}");
  NodePrefs loaded;
  loaded.rx_boosted_gain = 0;
  loaded.radio_fem_rxgain = 1;
  loaded.radio_fem_txgain = 0;

  ASSERT_TRUE(loaded.loadSerial(input));
  EXPECT_EQ(1, loaded.rx_boosted_gain);
  EXPECT_EQ(0, loaded.radio_fem_rxgain);
  EXPECT_EQ(1, loaded.radio_fem_txgain);
}
#endif

TEST(CompanionNodePrefs, LegacySettingsKeepRadioDelaysAndFemSeparate) {
  NodePrefs loaded;
  loaded.tx_delay_factor = 0.5f;
  loaded.direct_tx_delay_factor = 0.2f;
  ReplayStream input("{radio:{tx:11,rxgain:0,fem_rxgain:1},custom:{},gps:{en:0}}");
  ASSERT_TRUE(loaded.loadSerial(input));
  EXPECT_FLOAT_EQ(0.5f, loaded.tx_delay_factor);
  EXPECT_FLOAT_EQ(0.2f, loaded.direct_tx_delay_factor);
  EXPECT_EQ(11, loaded.tx_power_dbm);
  EXPECT_EQ(0, loaded.rx_boosted_gain);
  EXPECT_EQ(1, loaded.radio_fem_rxgain);
  EXPECT_EQ(0, loaded.gps_enabled);
}

TEST(CompanionNodePrefs, AgcIntervalSupportsMoreThan255Seconds) {
  NodePrefs prefs;
  auto* radio = prefs.getRadioPrefs();
  radio->setAgcResetInt(1020);
  EXPECT_EQ(1020, radio->getAgcResetInt());
  radio->setAgcResetInt(259);
  EXPECT_EQ(256, radio->getAgcResetInt());
}

TEST(CompanionNodePrefs, RejectsInvalidAirtimeWithoutChangingSavedValue) {
  NodePrefs prefs;
  prefs.airtime_factor = 1;
  char reply[160];
  for (const char* command : {"set af nan", "set af inf", "set af -1",
                            "set af 10", "set af 1garbage", "set af ", "set af    "}) {
    ASSERT_TRUE(prefs.getRadioPrefs()->handleCommand(command, 0, reply));
    EXPECT_EQ(0, strncmp(reply, "ERROR", 5));
    EXPECT_FLOAT_EQ(1, prefs.airtime_factor);
  }
  ASSERT_TRUE(prefs.getRadioPrefs()->handleCommand("set af 2.5 ", 0, reply));
  EXPECT_STREQ("OK", reply);
  EXPECT_FLOAT_EQ(2.5f, prefs.airtime_factor);
}

TEST(CompanionNodePrefs, AgcCommandsClampBeforeNarrowing) {
  NodePrefs prefs;
  char reply[160];
  auto* radio = prefs.getRadioPrefs();
  ASSERT_TRUE(radio->handleCommand("set agc.reset.interval 9999", 0, reply));
  EXPECT_EQ(1020, radio->getAgcResetInt());
  ASSERT_TRUE(radio->handleCommand("set agc.reset.interval -1", 0, reply));
  EXPECT_EQ(0, radio->getAgcResetInt());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
