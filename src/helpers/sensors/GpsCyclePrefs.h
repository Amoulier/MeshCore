#pragma once

#include <helpers/ConfigSerializer.h>
#include "GpsDutyCyclePolicy.h"

namespace gps {

namespace detail {
inline bool parseGpsUnsigned(const char* text, uint32_t& result) {
  if (text == nullptr || *text == '\0') return false;
  uint32_t value = 0;
  for (const char* cursor = text; *cursor != '\0'; ++cursor) {
    if (*cursor < '0' || *cursor > '9') return false;
    const uint32_t digit = static_cast<uint32_t>(*cursor - '0');
    if (value > (UINT32_MAX - digit) / 10U) return false;
    value = value * 10U + digit;
  }
  result = value;
  return true;
}
} // namespace detail

class GpsCyclePrefs : public ConfigSerializer {
  bool _invalid_serialized_value = false;

protected:
  void structure() override {
    if (_context->op() == READ) {
      // Avoid ConfigSerializer's atoi/atol narrowing: overflow or a suffix must
      // not turn an invalid stored opt-in/period into an apparently valid one.
      const char* keys[] = {"en", "acquire", "rest", "hold"};
      for (unsigned i = 0; i < 4; ++i) {
        if (!_context->keyMatch(getDepth(), keys[i])) continue;
        uint32_t value;
        if (!detail::parseGpsUnsigned(_context->getToken(), value) || (i == 0 && value > 1)) {
          _invalid_serialized_value = true;
          return;
        }
        if (i == 0) enabled = static_cast<uint8_t>(value);
        else if (i == 1) acquire = value;
        else if (i == 2) rest = value;
        else hold = value;
      }
      return;
    }
    def("en", enabled);
    def("acquire", acquire);
    def("rest", rest);
    def("hold", hold);
  }

public:
  uint8_t enabled = 0;
  uint32_t acquire = 120;
  uint32_t rest = 900;
  uint32_t hold = 5;

  GpsDutyCycleConfig config() const { return GpsDutyCycleConfig(acquire, rest, hold); }

  void normalize() {
    GpsDutyCyclePolicy validator;
    if (_invalid_serialized_value || enabled > 1 || !validator.configure(config())) {
      enabled = 0;
      const GpsDutyCycleConfig defaults;
      acquire = defaults.acquire_seconds;
      rest = defaults.rest_seconds;
      hold = defaults.fix_hold_seconds;
    }
    _invalid_serialized_value = false;
  }

  template <typename Sensors>
  bool apply(Sensors& sensors) const {
    return sensors.configureGpsCycle(config(), enabled != 0);
  }
};

} // namespace gps
