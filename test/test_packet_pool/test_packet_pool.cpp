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
  EXPECT_EQ(0, pool.getInboundTotal());
  EXPECT_EQ(nullptr, pool.getNextInbound(0));
  auto* packet = pool.allocNew();
  ASSERT_NE(nullptr, packet);
  EXPECT_EQ(nullptr, pool.allocNew());
  pool.free(packet);
  EXPECT_EQ(packet, pool.allocNew());
}

TEST(PacketPool, DelayedInboundRemainsPendingUntilDrained) {
  StaticPoolPacketManager pool(3);
  const mesh::PacketManager& manager = pool;
  auto* first = pool.allocNew();
  auto* later = pool.allocNew();
  ASSERT_NE(nullptr, first);
  ASSERT_NE(nullptr, later);
  pool.queueInbound(first, 100);
  pool.queueInbound(later, 500);

  EXPECT_EQ(2, manager.getInboundTotal());
  EXPECT_EQ(0, manager.getOutboundTotal());
  EXPECT_EQ(nullptr, pool.getNextInbound(99));
  EXPECT_EQ(2, manager.getInboundTotal());
  EXPECT_EQ(first, pool.getNextInbound(100));
  EXPECT_EQ(1, manager.getInboundTotal());
  pool.free(first);
  EXPECT_EQ(nullptr, pool.getNextInbound(499));
  EXPECT_EQ(1, manager.getInboundTotal());
  EXPECT_EQ(later, pool.getNextInbound(500));
  EXPECT_EQ(0, manager.getInboundTotal());
  pool.free(later);
  EXPECT_EQ(nullptr, pool.getNextInbound(501));
  EXPECT_EQ(3, pool.getFreeCount());
}

TEST(PacketPool, InboundAndOutboundRemainIndependentBeforeTheirDeadlines) {
  StaticPoolPacketManager pool(2);
  auto* incoming = pool.allocNew();
  auto* outgoing = pool.allocNew();
  ASSERT_NE(nullptr, incoming);
  ASSERT_NE(nullptr, outgoing);
  pool.queueInbound(incoming, 150);
  pool.queueOutbound(outgoing, 0, 250);

  EXPECT_EQ(1, pool.getInboundTotal());
  EXPECT_EQ(1, pool.getOutboundTotal());
  EXPECT_EQ(0, pool.getOutboundCount(100));
  EXPECT_EQ(nullptr, pool.getNextOutbound(100));
  EXPECT_EQ(nullptr, pool.getNextInbound(100));
  EXPECT_EQ(1, pool.getInboundTotal());
  EXPECT_EQ(1, pool.getOutboundTotal());
  EXPECT_EQ(incoming, pool.getNextInbound(150));
  pool.free(incoming);
  EXPECT_EQ(0, pool.getInboundTotal());
  EXPECT_EQ(1, pool.getOutboundTotal());
  EXPECT_EQ(outgoing, pool.getNextOutbound(250));
  pool.free(outgoing);
  EXPECT_EQ(0, pool.getOutboundTotal());
  EXPECT_EQ(2, pool.getFreeCount());
}

TEST(PacketPool, DelayedInboundStaysPendingAcrossClockWrap) {
  StaticPoolPacketManager pool(1);
  auto* packet = pool.allocNew();
  ASSERT_NE(nullptr, packet);
  pool.queueInbound(packet, 0x20U);

  EXPECT_EQ(nullptr, pool.getNextInbound(0xFFFFFFF0U));
  EXPECT_EQ(1, pool.getInboundTotal());
  EXPECT_EQ(nullptr, pool.getNextInbound(0x1FU));
  EXPECT_EQ(1, pool.getInboundTotal());
  EXPECT_EQ(packet, pool.getNextInbound(0x20U));
  EXPECT_EQ(0, pool.getInboundTotal());
  pool.free(packet);
  EXPECT_EQ(1, pool.getFreeCount());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
