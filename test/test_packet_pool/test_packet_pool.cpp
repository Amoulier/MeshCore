#include <gtest/gtest.h>
#include <helpers/StaticPoolPacketManager.h>

TEST(PacketPool, InvalidIndicesLeaveQueuedPacketIntact) {
  StaticPoolPacketManager pool(1);
  auto* packet = pool.allocNew();
  ASSERT_NE(nullptr, packet);
  pool.queueOutbound(packet, 0, 0);
  EXPECT_EQ(nullptr, pool.getOutboundByIdx(-1));
  EXPECT_EQ(nullptr, pool.getOutboundByIdx(1));
  EXPECT_EQ(nullptr, pool.removeOutboundByIdx(-1));
  EXPECT_EQ(nullptr, pool.removeOutboundByIdx(1));
  EXPECT_EQ(1, pool.getOutboundTotal());
  EXPECT_EQ(packet, pool.getNextOutbound(0));
  pool.free(packet);
  EXPECT_EQ(1, pool.getFreeCount());
}

TEST(PacketPool, NullOperationsDoNotConsumeCapacity) {
  StaticPoolPacketManager pool(1);
  pool.free(nullptr);
  pool.queueOutbound(nullptr, 0, 0);
  pool.queueInbound(nullptr, 0);
  EXPECT_EQ(1, pool.getFreeCount());
  EXPECT_EQ(0, pool.getOutboundTotal());
  EXPECT_EQ(nullptr, pool.getNextInbound(0));
  auto* packet = pool.allocNew();
  ASSERT_NE(nullptr, packet);
  EXPECT_EQ(nullptr, pool.allocNew());
  pool.free(packet);
  EXPECT_EQ(packet, pool.allocNew());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
