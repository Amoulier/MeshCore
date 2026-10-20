#pragma once

#include <stddef.h>
#include <MeshCore.h>
#include "LPPDataHelpers.h"

namespace telemetry_subscription {
// Match Mesh::createDatagram's conservative padding bound and reserve the
// pushed response's tag and timestamp before admitting any sensor records.
constexpr size_t MAX_DATA_SIZE = MAX_PACKET_PAYLOAD - CIPHER_MAC_SIZE -
    (CIPHER_BLOCK_SIZE - 1) - 8;
// Thresholds describe scalar channels; composite GPS/vector records cannot be
// compared as one float. Validate the complete list before updating a client.
inline bool validThresholds(const uint8_t* data, size_t length) {
  if (!data || length < 3) return false;
  size_t offset = 0;
  while (offset < length) {
    if (length - offset < 3 || data[offset] == 0) return false;
    const uint8_t type = data[offset + 1];
    switch (type) {
      case LPP_DIGITAL_INPUT: case LPP_DIGITAL_OUTPUT:
      case LPP_ANALOG_INPUT: case LPP_ANALOG_OUTPUT:
      case LPP_GENERIC_SENSOR: case LPP_LUMINOSITY: case LPP_PRESENCE:
      case LPP_TEMPERATURE: case LPP_RELATIVE_HUMIDITY:
      case LPP_BAROMETRIC_PRESSURE: case LPP_VOLTAGE: case LPP_CURRENT:
      case LPP_FREQUENCY: case LPP_PERCENTAGE: case LPP_ALTITUDE:
      case LPP_CONCENTRATION: case LPP_POWER: case LPP_DISTANCE:
      case LPP_ENERGY: case LPP_DIRECTION: case LPP_UNIXTIME: case LPP_SWITCH:
        break;
      default: return false;
    }
    const size_t size = LPPData::getDataSize(type);
    if (size > length - offset - 2 || (data[offset + 2] & 0x80 && LPPData::isSigned(type))) return false;
    offset += 2 + size;
  }
  return offset == length;
}

inline bool validRequest(const uint8_t* payload, size_t length, size_t capacity) {
  return payload && length >= 8 && payload[6] == 0 &&
      payload[7] <= capacity && length - 8 == payload[7] &&
      validThresholds(payload + 8, payload[7]);
}
}
