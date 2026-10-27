#pragma once

#include <Arduino.h>
#include "SuperbaseSupport.h"

namespace superbase {

class BQ25185Inputs {
  uint8_t _stat1, _stat2;
public:
  BQ25185Inputs(uint8_t stat1, uint8_t stat2) : _stat1(stat1), _stat2(stat2) {}
  void begin() {
    // STAT1 is open-drain; the Base board has no external pull-up on this input.
    pinMode(_stat1, INPUT_PULLUP);
    pinMode(_stat2, INPUT);
  }
  ChargeState read(bool powered) const {
    return chargeState(powered, digitalRead(_stat1), digitalRead(_stat2));
  }
};

} // namespace superbase
