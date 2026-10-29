#pragma once

#include <Packet.h>
#include <stddef.h>

namespace mesh {

enum RepeaterAnonRequestType : uint8_t {
  REPEATER_ANON_REQ_REGIONS = 0x01,
  REPEATER_ANON_REQ_OWNER = 0x02,
  REPEATER_ANON_REQ_BASIC = 0x03,
};

// Decrypted requests contain timestamp(4), type/password(1), then optionally
// reply-path-len(1) and path bytes. The caller appends a NUL for login passwords.
inline bool isCompleteRepeaterAnonRequest(const uint8_t* data, size_t len, bool is_direct) {
  if (data == nullptr || len < 5 || len >= MAX_PACKET_PAYLOAD) return false;

  // Only these direct anonymous queries carry a reply path. Login passwords,
  // including an empty password, and other request types retain their dispatch.
  const uint8_t type = data[4];
  if (!is_direct || (type != REPEATER_ANON_REQ_REGIONS &&
                     type != REPEATER_ANON_REQ_OWNER &&
                     type != REPEATER_ANON_REQ_BASIC)) return true;

  if (len < 6 || !Packet::isValidPathLen(data[5])) return false;
  const size_t path_bytes = (data[5] & 63) * ((data[5] >> 6) + 1);
  return path_bytes <= len - 6;
}

}
