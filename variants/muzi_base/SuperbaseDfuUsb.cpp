#if defined(MUZI_BASE_SUPERIO) && defined(NRF52_PLATFORM)
#include "SuperbaseDFU.h"

// This is the same port hook used by Bluefruit's SOC handler. Call it while
// the application's TinyUSB queue and SoftDevice are still alive, not from
// the bootloader, whose BLE-only path never initializes TinyUSB.
extern "C" void tusb_hal_nrf_power_event(uint32_t event);

bool superbaseDfuPrepareUsb() {
#ifdef USE_TINYUSB
  // Prevent a later power-ready event from enabling USBD again during handoff.
  if (sd_power_usbdetected_enable(false) != NRF_SUCCESS) return false;
  if (sd_power_usbpwrrdy_enable(false) != NRF_SUCCESS) {
    sd_power_usbdetected_enable(true);
    return false;
  }
  if (sd_power_usbremoved_enable(false) != NRF_SUCCESS) {
    sd_power_usbdetected_enable(true);
    sd_power_usbpwrrdy_enable(true);
    return false;
  }
  // NRFX_POWER_USB_EVT_REMOVED = 1. The driver disables pull-up, IRQs and
  // USBD and releases its HFCLK request before queuing an unplugged event.
  tusb_hal_nrf_power_event(1);
#endif
  return true;
}
#endif
