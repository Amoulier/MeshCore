#pragma once
#include <stdint.h>

// RTC boundary only. Production MicroNMEA provider calls the real interface's
// uint32 epoch setter; no mesh/radio implementation is needed by this suite.
namespace mesh {
class RTCClock {
public:
  virtual ~RTCClock() = default;
  virtual uint32_t getCurrentTime() = 0;
  virtual void setCurrentTime(uint32_t epoch) = 0;
};
}
