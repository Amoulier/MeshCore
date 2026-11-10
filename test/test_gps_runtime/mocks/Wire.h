#pragma once
#include <cstdint>
class TwoWire {
public:
  int begin_calls = 0, probes = 0;
  void begin(int = -1, int = -1, int = 100000) { begin_calls++; }
  void beginTransmission(uint8_t) { probes++; }
  int endTransmission() { return 4; }
};
inline TwoWire Wire, Wire1;
