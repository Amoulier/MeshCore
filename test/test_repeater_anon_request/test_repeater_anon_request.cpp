#include <gtest/gtest.h>
#include <array>
#include "helpers/RepeaterAnonRequest.h"

using namespace mesh;

class RepeaterAnonRequest : public ::testing::Test {
protected:
  std::array<uint8_t, MAX_PACKET_PAYLOAD> data{};

  void query(uint8_t type, uint8_t encoded_path) {
    data.fill(0);
    data[4] = type;
    data[5] = encoded_path;
  }
};

TEST_F(RepeaterAnonRequest, RejectsMissingHeaderAndMissingSpaceForPasswordTerminator) {
  EXPECT_FALSE(isCompleteRepeaterAnonRequest(nullptr, 16, true));
  for (size_t len = 0; len < 5; ++len) {
    EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), len, false)) << len;
    EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), len, true)) << len;
  }
  EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 5, true));  // empty login
  EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), data.size(), true));
}

TEST_F(RepeaterAnonRequest, RejectsTruncatedPathInEachAnonymousQuery) {
  for (auto type : {REPEATER_ANON_REQ_REGIONS, REPEATER_ANON_REQ_OWNER, REPEATER_ANON_REQ_BASIC}) {
    query(type, 63);
    EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 5, true)) << type;
    EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 16, true)) << type;
    EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 68, true)) << type;
    EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 69, true)) << type;
    // Encryption zero-pads to a full block; padding remains received plaintext.
    EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 80, true)) << type;
  }
}

TEST_F(RepeaterAnonRequest, AcceptsZeroHopPathsForAllSupportedHashSizes) {
  for (auto type : {REPEATER_ANON_REQ_REGIONS, REPEATER_ANON_REQ_OWNER, REPEATER_ANON_REQ_BASIC}) {
    for (uint8_t path : {0x00, 0x40, 0x80}) {
      query(type, path);
      EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 6, true)) << type << ':' << path;
    }
  }
}

TEST_F(RepeaterAnonRequest, UsesByteLengthRatherThanHopCountForMultibytePaths) {
  query(REPEATER_ANON_REQ_BASIC, 0x60);  // 32 two-byte hashes: 64 path bytes
  EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 38, true));
  EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 69, true));
  EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 70, true));
  EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 80, true));

  query(REPEATER_ANON_REQ_BASIC, 0x95);  // 21 three-byte hashes: 63 path bytes
  EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 27, true));
  EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 68, true));
  EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 69, true));
}

TEST_F(RepeaterAnonRequest, RejectsReservedHashModeAndPathsBeyondStorage) {
  for (uint8_t encoded : {0xc0, 0xc1, 0xff, 0x61, 0x96}) {
    query(REPEATER_ANON_REQ_OWNER, encoded);
    EXPECT_FALSE(isCompleteRepeaterAnonRequest(data.data(), 160, true)) << encoded;
  }
}

TEST_F(RepeaterAnonRequest, PreservesLoginDispatchWithoutInterpretingPasswordBytesAsPaths) {
  for (uint8_t first : {0x00, 0x20, 0x7e, 0xff}) {
    data.fill(0xff);  // would be a reserved path encoding if interpreted as one
    data[4] = first;
    EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 5, true)) << first;
    EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 16, false)) << first;
    EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 16, true)) << first;
  }
}

TEST_F(RepeaterAnonRequest, LeavesUnsupportedTypesAndFloodDispatchUnchanged) {
  data.fill(0xff);
  data[4] = 0x04;  // dispatch rejects an unknown non-login request
  EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 5, true));
  for (auto type : {REPEATER_ANON_REQ_REGIONS, REPEATER_ANON_REQ_OWNER, REPEATER_ANON_REQ_BASIC}) {
    data[4] = type;
    // These types have never been handled on flood routes; the guard must not
    // reinterpret that route or apply the direct-only path requirement there.
    EXPECT_TRUE(isCompleteRepeaterAnonRequest(data.data(), 5, false)) << type;
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
