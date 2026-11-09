#pragma once

#include <Packet.h>
#include <stddef.h>

namespace mesh {

struct DecodedPeerPathPayload {
  uint8_t encoded_path_len;
  uint8_t path_bytes;
  uint8_t extra_type;
  uint8_t extra_len;
};

// Plaintext layout: encoded path length(1), path bytes, extra type(1), extra.
// Extra may include cipher padding; retain it for the existing callbacks.
inline bool decodePeerPathPayload(const uint8_t* data, size_t len,
                                  DecodedPeerPathPayload& decoded) {
  if (data == nullptr || len < 2 || len > MAX_PACKET_PAYLOAD) return false;

  const uint8_t encoded_path_len = data[0];
  if (!Packet::isValidPathLen(encoded_path_len)) return false;

  const size_t path_bytes = (encoded_path_len & 63U) * ((encoded_path_len >> 6) + 1U);
  // Subtract only after checking len >= 2. Both framing bytes must fit before
  // reading extra_type or calculating the remaining length.
  if (path_bytes > len - 2) return false;

  decoded.encoded_path_len = encoded_path_len;
  decoded.path_bytes = static_cast<uint8_t>(path_bytes);
  decoded.extra_type = data[1 + path_bytes] & 0x0F;
  decoded.extra_len = static_cast<uint8_t>(len - 2 - path_bytes);
  return true;
}

} // namespace mesh
