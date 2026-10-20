#include <gtest/gtest.h>
#include <Utils.h>
#include <cstring>

TEST(UtilsDecrypt, RejectsInvalidBlockLengthsWithoutWritingDestination) {
  uint8_t secret[PUB_KEY_SIZE] = {};
  uint8_t source[MAX_PACKET_PAYLOAD] = {};
  uint8_t destination[MAX_PACKET_PAYLOAD];
  for (int length = 0; length <= MAX_PACKET_PAYLOAD; ++length) {
    if (length > CIPHER_MAC_SIZE &&
        (length - CIPHER_MAC_SIZE) % CIPHER_BLOCK_SIZE == 0) continue;
    memset(destination, 0xA5, sizeof(destination));
    EXPECT_EQ(0, mesh::Utils::MACThenDecrypt(secret, destination, source, length));
    for (auto byte : destination) EXPECT_EQ(0xA5, byte);
  }
}
