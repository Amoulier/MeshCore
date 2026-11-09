#include <gtest/gtest.h>
#include <helpers/PeerPathPayload.h>
#include <array>
#include <vector>

namespace {

using mesh::DecodedPeerPathPayload;
using mesh::decodePeerPathPayload;

TEST(PeerPathPayload, RejectsNullEmptyAndMissingExtraType) {
  const uint8_t header_only[] = {0};
  DecodedPeerPathPayload decoded = {};
  EXPECT_FALSE(decodePeerPathPayload(nullptr, 0, decoded));
  EXPECT_FALSE(decodePeerPathPayload(nullptr, 2, decoded));
  EXPECT_FALSE(decodePeerPathPayload(header_only, 0, decoded));
  EXPECT_FALSE(decodePeerPathPayload(header_only, sizeof(header_only), decoded));
}

TEST(PeerPathPayload, ZeroHopNeedsBothFramingBytesForEveryHashWidth) {
  for (uint8_t hash_size = 1; hash_size <= 3; ++hash_size) {
    const uint8_t data[] = {static_cast<uint8_t>((hash_size - 1) << 6), 0x0F};
    DecodedPeerPathPayload decoded = {};
    ASSERT_TRUE(decodePeerPathPayload(data, sizeof(data), decoded));
    EXPECT_EQ(data[0], decoded.encoded_path_len);
    EXPECT_EQ(0, decoded.path_bytes);
    EXPECT_EQ(0x0F, decoded.extra_type);
    EXPECT_EQ(0, decoded.extra_len);
  }
}

TEST(PeerPathPayload, RejectsEveryTruncatedPrefixForOneTwoAndThreeByteHashes) {
  for (uint8_t hash_size = 1; hash_size <= 3; ++hash_size) {
    const size_t path_bytes = 2 * hash_size;
    std::vector<uint8_t> data(path_bytes + 2, 0xA5);
    data[0] = static_cast<uint8_t>(((hash_size - 1) << 6) | 2);
    data.back() = 0x03;
    for (size_t len = 0; len < data.size(); ++len) {
      DecodedPeerPathPayload decoded = {};
      EXPECT_FALSE(decodePeerPathPayload(data.data(), len, decoded))
          << "hash_size=" << static_cast<unsigned>(hash_size) << " len=" << len;
    }
    DecodedPeerPathPayload decoded = {};
    ASSERT_TRUE(decodePeerPathPayload(data.data(), data.size(), decoded));
    EXPECT_EQ(path_bytes, decoded.path_bytes);
    EXPECT_EQ(0, decoded.extra_len);
  }
}

TEST(PeerPathPayload, UsesPlaintextLengthRatherThanBackingBufferCapacity) {
  std::array<uint8_t, MAX_PACKET_PAYLOAD> data = {};
  data.fill(0xA5);
  data[0] = 0x94;  // twenty 3-byte hops, valid encoding but needs 62 bytes.
  DecodedPeerPathPayload decoded = {};
  EXPECT_FALSE(decodePeerPathPayload(data.data(), 16, decoded));
  EXPECT_FALSE(decodePeerPathPayload(data.data(), 61, decoded));
  ASSERT_TRUE(decodePeerPathPayload(data.data(), 62, decoded));
  EXPECT_EQ(60, decoded.path_bytes);
  EXPECT_EQ(0, decoded.extra_len);
}

TEST(PeerPathPayload, AcceptsMaximumPathsWithinTheProtocolCapacity) {
  const uint8_t hop_counts[] = {63, 32, 21};
  for (uint8_t hash_size = 1; hash_size <= 3; ++hash_size) {
    const size_t path_bytes = hash_size * hop_counts[hash_size - 1];
    std::vector<uint8_t> data(path_bytes + 2, 0);
    data[0] = static_cast<uint8_t>(((hash_size - 1) << 6) | hop_counts[hash_size - 1]);
    DecodedPeerPathPayload decoded = {};
    ASSERT_TRUE(decodePeerPathPayload(data.data(), data.size(), decoded));
    EXPECT_EQ(path_bytes, decoded.path_bytes);
    EXPECT_EQ(0, decoded.extra_len);
  }
}

TEST(PeerPathPayload, RejectsOversizeAndReservedPathEncodings) {
  std::array<uint8_t, MAX_PACKET_PAYLOAD> data = {};
  const uint8_t invalid_lengths[] = {0x61, 0x96, 0xC0, 0xC1, 0xFE, 0xFF};
  for (uint8_t encoded : invalid_lengths) {
    data[0] = encoded;
    DecodedPeerPathPayload decoded = {};
    EXPECT_FALSE(decodePeerPathPayload(data.data(), data.size(), decoded))
        << "encoded=" << static_cast<unsigned>(encoded);
  }
}

TEST(PeerPathPayload, PreservesExtraBytesIncludingCipherPadding) {
  std::array<uint8_t, 16> data = {};
  data[0] = 2;  // two 1-byte hops
  data[1] = 0x12;
  data[2] = 0x34;
  data[3] = 0xF7;  // high bits are reserved; low nibble remains the type.
  data[4] = 0x42;
  DecodedPeerPathPayload decoded = {};
  ASSERT_TRUE(decodePeerPathPayload(data.data(), data.size(), decoded));
  EXPECT_EQ(2, decoded.path_bytes);
  EXPECT_EQ(7, decoded.extra_type);
  EXPECT_EQ(12, decoded.extra_len);
  EXPECT_EQ(0x42, data[2 + decoded.path_bytes]);
  EXPECT_EQ(0, data[2 + decoded.path_bytes + decoded.extra_len - 1]);
}

TEST(PeerPathPayload, RetainsEveryExtraTypeAndItsReservedHighBitsMask) {
  for (uint8_t type = 0; type < 16; ++type) {
    const uint8_t data[] = {0, static_cast<uint8_t>(0xA0 | type)};
    DecodedPeerPathPayload decoded = {};
    ASSERT_TRUE(decodePeerPathPayload(data, sizeof(data), decoded));
    EXPECT_EQ(type, decoded.extra_type);
    EXPECT_EQ(0, decoded.extra_len);
  }
}

TEST(PeerPathPayload, AcceptsFullPlaintextBufferWithoutTruncatingExtraLength) {
  std::array<uint8_t, MAX_PACKET_PAYLOAD> data = {};
  DecodedPeerPathPayload decoded = {};
  ASSERT_TRUE(decodePeerPathPayload(data.data(), data.size(), decoded));
  EXPECT_EQ(MAX_PACKET_PAYLOAD - 2, decoded.extra_len);
}

TEST(PeerPathPayload, RejectsOversizePlaintextAndLeavesOutputUntouchedOnFailure) {
  std::array<uint8_t, MAX_PACKET_PAYLOAD + 1> data = {};
  DecodedPeerPathPayload decoded = {0xA1, 0xA2, 0xA3, 0xA4};
  EXPECT_FALSE(decodePeerPathPayload(data.data(), data.size(), decoded));
  EXPECT_EQ(0xA1, decoded.encoded_path_len);
  EXPECT_EQ(0xA2, decoded.path_bytes);
  EXPECT_EQ(0xA3, decoded.extra_type);
  EXPECT_EQ(0xA4, decoded.extra_len);
  data[0] = 1;
  EXPECT_FALSE(decodePeerPathPayload(data.data(), 2, decoded));
  EXPECT_EQ(0xA1, decoded.encoded_path_len);
  EXPECT_EQ(0xA2, decoded.path_bytes);
  EXPECT_EQ(0xA3, decoded.extra_type);
  EXPECT_EQ(0xA4, decoded.extra_len);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
