#include <cassert>
#include <iostream>
#include "SuperbaseDFU.h"

extern "C" void tusb_hal_nrf_power_event(uint32_t event) {
  assert(event == 1);
  // No subsequent power-ready notification may resurrect USB during handoff.
  assert(!mock_usb_events[0] && !mock_usb_events[1] && !mock_usb_events[2]);
  ++mock_usb_removals;
}

int main() {
#ifdef USE_TINYUSB
  for (int failure = 0; failure < 3; ++failure) {
    mock_usb_fail_event = failure;
    assert(!superbaseDfuPrepareUsb());
    assert(mock_usb_removals == 0);
    assert(mock_usb_events[0] && mock_usb_events[1] && mock_usb_events[2]);
  }
  mock_usb_fail_event = -1;
  assert(superbaseDfuPrepareUsb());
  assert(mock_usb_removals == 1);
#else
  assert(superbaseDfuPrepareUsb());
  assert(mock_usb_removals == 0);
  assert(mock_usb_events[0] && mock_usb_events[1] && mock_usb_events[2]);
#endif
  std::cout << "PASS: production DFU USB shutdown, power-event ordering and failure rollback\n";
}
