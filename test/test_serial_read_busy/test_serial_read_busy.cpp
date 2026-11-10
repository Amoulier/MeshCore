#include <gtest/gtest.h>
#include <deque>
#include <helpers/ArduinoSerialInterface.h>
#include <helpers/MultiSerialInterface.h>
#include <helpers/esp32/SerialWifiInterface.h>

namespace {
class ByteStream : public Stream {
  std::deque<uint8_t> bytes;
public:
  int available() override { return static_cast<int>(bytes.size()); }
  int read() override {
    if (bytes.empty()) return -1;
    int value = bytes.front(); bytes.pop_front(); return value;
  }
  void inject(std::initializer_list<uint8_t> data) { bytes.insert(bytes.end(), data); }
};

class SerialReadBusy : public ::testing::Test {
protected:
  ByteStream stream;
  ArduinoSerialInterface serial;
  uint8_t output[MAX_FRAME_SIZE] = {};
  void SetUp() override { g_mock_millis = 100; serial.begin(stream); serial.enable(); }
};

TEST_F(SerialReadBusy, FragmentedHeaderAndPayloadStayBusyUntilComplete) {
  EXPECT_FALSE(serial.isReadBusy());
  stream.inject({'<'});
  EXPECT_TRUE(serial.isReadBusy());
  EXPECT_EQ(0U, serial.checkRecvFrame(output));
  EXPECT_TRUE(serial.isReadBusy());
  stream.inject({2});
  EXPECT_EQ(0U, serial.checkRecvFrame(output));
  stream.inject({0, 0x12});
  EXPECT_EQ(0U, serial.checkRecvFrame(output));
  EXPECT_TRUE(serial.isReadBusy());
  stream.inject({0x34});
  EXPECT_EQ(2U, serial.checkRecvFrame(output));
  EXPECT_EQ(0x12, output[0]); EXPECT_EQ(0x34, output[1]);
  EXPECT_FALSE(serial.isReadBusy());
}

TEST_F(SerialReadBusy, QueuedSecondFramePreventsAnIdleGap) {
  stream.inject({'<', 1, 0, 0x12, '<', 1, 0, 0x34});
  EXPECT_EQ(1U, serial.checkRecvFrame(output));
  EXPECT_EQ(0x12, output[0]);
  EXPECT_TRUE(serial.isReadBusy());
  EXPECT_EQ(1U, serial.checkRecvFrame(output));
  EXPECT_EQ(0x34, output[0]);
  EXPECT_FALSE(serial.isReadBusy());
}

TEST_F(SerialReadBusy, InactivityExpiresAFragmentAndRecoversAtTheNextFrame) {
  stream.inject({'<', 2, 0, 0x12});
  EXPECT_EQ(0U, serial.checkRecvFrame(output));
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  EXPECT_TRUE(serial.isReadBusy());
  g_mock_millis++;
  EXPECT_FALSE(serial.isReadBusy());
  stream.inject({'<', 1, 0, 0x56});
  EXPECT_TRUE(serial.isReadBusy());
  EXPECT_EQ(1U, serial.checkRecvFrame(output));
  EXPECT_EQ(0x56, output[0]);
  EXPECT_FALSE(serial.isReadBusy());
}

TEST_F(SerialReadBusy, EachFragmentRenewsTheInactivityDeadline) {
  stream.inject({'<'}); serial.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  stream.inject({1, 0}); serial.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  EXPECT_TRUE(serial.isReadBusy());
  stream.inject({0x78});
  EXPECT_EQ(1U, serial.checkRecvFrame(output));
  EXPECT_EQ(0x78, output[0]);
}

TEST_F(SerialReadBusy, DeadlineSurvivesMillisRollover) {
  g_mock_millis = 0xfffffff0U;
  stream.inject({'<'}); serial.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  EXPECT_TRUE(serial.isReadBusy());
  g_mock_millis++;
  EXPECT_FALSE(serial.isReadBusy());
  EXPECT_EQ(0U, serial.checkRecvFrame(output));
}

TEST_F(SerialReadBusy, DisabledAndUnboundInterfacesDoNotInhibitSleep) {
  ArduinoSerialInterface unbound;
  unbound.enable();
  EXPECT_FALSE(unbound.isReadBusy());
  EXPECT_EQ(0U, unbound.checkRecvFrame(output));
  stream.inject({'<'}); serial.checkRecvFrame(output);
  serial.disable();
  EXPECT_FALSE(serial.isReadBusy());
  serial.enable();
  EXPECT_FALSE(serial.isReadBusy());
}

class WifiReadBusy : public ::testing::Test {
protected:
  SerialWifiInterface wifi;
  WiFiClient peer;
  uint8_t output[MAX_FRAME_SIZE] = {};
  void SetUp() override {
    g_mock_millis = 100; mock_wifi::pending.reset();
    wifi.enable(); peer = connectMockWiFi();
    EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  }
};

TEST_F(WifiReadBusy, TracksSocketBytesAndAConsumedHeaderUntilPayloadArrives) {
  peer.inject({'<', 2});
  EXPECT_TRUE(wifi.isReadBusy());
  EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  peer.inject({0});
  EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  EXPECT_TRUE(wifi.isReadBusy());
  peer.inject({0x12});
  EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  peer.inject({0x34});
  EXPECT_EQ(2U, wifi.checkRecvFrame(output));
  EXPECT_EQ(0x12, output[0]); EXPECT_EQ(0x34, output[1]);
  EXPECT_FALSE(wifi.isReadBusy());
}

TEST_F(WifiReadBusy, ExpiredPartialHeaderIsDrainedWithoutBlockingTheNextFrame) {
  peer.inject({'<', 2}); wifi.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS;
  EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  EXPECT_FALSE(wifi.isReadBusy());
  peer.inject({'<', 1, 0, 0x56});
  EXPECT_EQ(1U, wifi.checkRecvFrame(output));
  EXPECT_EQ(0x56, output[0]);
}

TEST_F(WifiReadBusy, IncompleteBufferedPayloadTimesOutAcrossRollover) {
  g_mock_millis = 0xfffffff0U;
  peer.inject({'<', 2, 0, 0x12}); wifi.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  EXPECT_TRUE(wifi.isReadBusy());
  EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  g_mock_millis++;
  EXPECT_EQ(0U, wifi.checkRecvFrame(output));
  EXPECT_FALSE(wifi.isReadBusy());
}

TEST_F(WifiReadBusy, IncomingProgressRenewsTheDeadlineAndDisconnectClearsIt) {
  peer.inject({'<', 2, 0}); wifi.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  peer.inject({0x12}); wifi.checkRecvFrame(output);
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS - 1;
  EXPECT_TRUE(wifi.isReadBusy());
  peer.stop(); wifi.checkRecvFrame(output);
  EXPECT_FALSE(wifi.isReadBusy());
}

TEST_F(SerialReadBusy, MultiAggregatesRealEnabledParsersAndIgnoresDisabledOnes) {
  SerialWifiInterface wifi;
  MultiSerialInterface multi;
  multi.addInterface(InterfaceType::USB, &serial);
  multi.addInterface(InterfaceType::WiFi, &wifi);
  multi.enable();
  stream.inject({'<'}); multi.checkRecvFrame(output);
  EXPECT_TRUE(multi.isReadBusy());
  serial.disable();
  EXPECT_FALSE(multi.isReadBusy());
  auto peer = connectMockWiFi(); peer.inject({'<', 1});
  multi.checkRecvFrame(output);
  EXPECT_TRUE(multi.isReadBusy());
  g_mock_millis += SERIAL_RX_FRAME_TIMEOUT_MILLIS;
  multi.checkRecvFrame(output);
  EXPECT_FALSE(multi.isReadBusy());
  multi.disable();
  peer.inject({0});
  EXPECT_FALSE(multi.isReadBusy());
}
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
