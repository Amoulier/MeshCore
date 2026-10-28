#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include "SuperbaseDfuPeer.h"

int main() {
  std::array<uint8_t, sizeof(superbase::muzi_peer_verifier)> verifier;
  std::memcpy(verifier.data(), superbase::muzi_peer_verifier, verifier.size());
  std::array<uint8_t, 64> record;
  record.fill(0xEE);
  for (unsigned i = 0; i < 60; ++i) record[i] = uint8_t(i);
  record[60] = 0xA4; record[61] = 0xBA; // CRC16-CCITT(00..3B, seed FFFF).
  const auto original = record;
  for (size_t i = 0; i < verifier.size(); ++i) {
    verifier[i] ^= 1;
    assert(!superbase::adaptDfuPeer(record.data(), verifier.data()));
    assert(record == original); // Unknown loaders must remain untouched.
    verifier[i] ^= 1;
  }
  assert(superbase::adaptDfuPeer(record.data(), verifier.data()));
  // The bootloader now reads its CRC at +0 and all 60 peer bytes at +2.
  assert(record[0] == 0xA4 && record[1] == 0xBA);
  for (unsigned i = 0; i < 60; ++i) assert(record[i + 2] == i);
  assert(record[62] == 0xEE && record[63] == 0xEE);
  std::cout << "PASS: exact bootloader ABI match, unknown-loader isolation and overlapping peer relocation\n";
}
