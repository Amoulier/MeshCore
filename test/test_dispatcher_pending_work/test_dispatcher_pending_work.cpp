#include <gtest/gtest.h>
#include <Dispatcher.h>
#include <helpers/StaticPoolPacketManager.h>

namespace {

class TestClock : public mesh::MillisecondClock {
public:
  unsigned long now = 100;
  unsigned long getMillis() override { return now; }
};

class TestRadio : public mesh::Radio {
public:
  bool allow_send = true;
  bool complete = false;
  bool receive_mode = true;
  int attempts = 0;
  int finished = 0;

  int recvRaw(uint8_t*, int) override { return 0; }
  uint32_t getEstAirtimeFor(int) override { return 100; }
  float packetScore(float, int) override { return 1.0f; }
  bool startSendRaw(const uint8_t*, int) override {
    attempts++;
    if (!allow_send) return false;
    receive_mode = false;
    return true;
  }
  bool isSendComplete() override { return complete; }
  void onSendFinished() override {
    finished++;
    receive_mode = true;
  }
  bool isInRecvMode() const override { return receive_mode; }
};

class TestDispatcher : public mesh::Dispatcher {
protected:
  mesh::DispatcherAction onRecvPacket(mesh::Packet*) override { return ACTION_RELEASE; }

public:
  TestDispatcher(TestRadio& radio, TestClock& clock, StaticPoolPacketManager& pool)
      : mesh::Dispatcher(radio, clock, pool) {}
};

class DispatcherPendingWork : public ::testing::Test {
protected:
  TestClock clock;
  TestRadio radio;
  StaticPoolPacketManager pool{1};
  TestDispatcher dispatcher{radio, clock, pool};

  void SetUp() override { dispatcher.begin(); }

  void queuePacket() {
    auto* packet = dispatcher.obtainNewPacket();
    ASSERT_NE(nullptr, packet);
    packet->header = ROUTE_TYPE_DIRECT | (PAYLOAD_TYPE_RAW_CUSTOM << PH_TYPE_SHIFT);
    packet->payload_len = 1;
    packet->payload[0] = 0x42;
    dispatcher.sendPacket(packet, 0);
  }
};

TEST_F(DispatcherPendingWork, ActiveSendRemainsPendingAfterLeavingTheQueue) {
  EXPECT_FALSE(dispatcher.isTransmitting());
  queuePacket();
  EXPECT_EQ(1, pool.getOutboundTotal());
  EXPECT_FALSE(dispatcher.isTransmitting());

  clock.now++;
  dispatcher.loop();
  EXPECT_EQ(1, radio.attempts);
  EXPECT_EQ(0, pool.getOutboundTotal());
  EXPECT_EQ(0, pool.getInboundTotal());
  EXPECT_EQ(0, pool.getFreeCount());
  EXPECT_TRUE(dispatcher.isTransmitting());

  clock.now += 20;
  dispatcher.loop();
  EXPECT_TRUE(dispatcher.isTransmitting());
  EXPECT_EQ(0, radio.finished);

  radio.complete = true;
  clock.now++;
  dispatcher.loop();
  EXPECT_FALSE(dispatcher.isTransmitting());
  EXPECT_EQ(1, radio.finished);
  EXPECT_EQ(1, pool.getFreeCount());
  EXPECT_EQ(1U, dispatcher.getNumSentDirect());
}

TEST_F(DispatcherPendingWork, FailedSendStartDoesNotLeaveActiveWork) {
  radio.allow_send = false;
  queuePacket();
  clock.now++;
  dispatcher.loop();

  EXPECT_EQ(1, radio.attempts);
  EXPECT_FALSE(dispatcher.isTransmitting());
  EXPECT_EQ(0, pool.getOutboundTotal());
  EXPECT_EQ(1, pool.getFreeCount());
  EXPECT_EQ(0, radio.finished);
  EXPECT_EQ(0U, dispatcher.getNumSentDirect());
}

TEST_F(DispatcherPendingWork, TimedOutSendReleasesActiveWorkWithoutACompletionInterrupt) {
  queuePacket();
  clock.now++;
  dispatcher.loop();
  ASSERT_TRUE(dispatcher.isTransmitting());

  // The radio reports 100 ms airtime; Dispatcher allows 150 ms for completion.
  clock.now += 151;
  dispatcher.loop();
  EXPECT_FALSE(dispatcher.isTransmitting());
  EXPECT_EQ(1, radio.finished);
  EXPECT_EQ(0, pool.getOutboundTotal());
  EXPECT_EQ(1, pool.getFreeCount());
  EXPECT_EQ(0U, dispatcher.getNumSentDirect());
}

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
