#include <Arduino.h>
#include "target.h"

HeltecV4Board board;

#if defined(P_LORA_SCLK)
static SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

#if ENV_INCLUDE_GPS
#include <helpers/sensors/MicroNMEALocationProvider.h>
MicroNMEALocationProvider nmea(Serial1, &rtc_clock);
EnvironmentSensorManager sensors(nmea);
#else
EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
#if defined(HELTEC_V4_KEEP_ACCESSORY_RAIL_ON) && HELTEC_V4_KEEP_ACCESSORY_RAIL_ON
DISPLAY_CLASS display(NULL);
#else
DISPLAY_CLASS display(&board.periph_power);
#endif
MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

bool heltecV4SetDisplayEnabled(bool enabled)
{
#ifdef DISPLAY_CLASS
  return display.setPersistentlyDisabled(!enabled);
#else
  (void)enabled;
  return false;
#endif
}

int8_t heltecV4GetDisplayDisabled()
{
#ifdef DISPLAY_CLASS
  return display.isPersistentlyDisabled() ? 1 : 0;
#else
  return -1;
#endif
}

bool platformHandleLongPress(int8_t pin)
{
#ifdef DISPLAY_CLASS
  if (pin == PIN_USER_BTN && display.isPersistentlyDisabled()) {
    display.setPersistentlyDisabled(false);
    return true;
  }
#else
  (void)pin;
#endif
  return false;
}

void heltecV4CriticalPreSleep()
{
#ifdef DISPLAY_CLASS
  display.turnOff();
#endif
  radio_driver.powerOff();
#if ENV_INCLUDE_GPS
  LocationProvider *location = sensors.getLocationProvider();
  if (location != NULL) {
    location->stop();
  }
#endif
}

bool radio_init()
{
  fallback_clock.begin();
  rtc_clock.begin(Wire);

#if defined(P_LORA_SCLK)
  const bool initialized = radio.std_init(&spi);
#else
  const bool initialized = radio.std_init();
#endif
  if (initialized) {
    // LORA_TX_POWER is the desired antenna output. The board maps it to the
    // safe SX1262 input required by the detected GC1109 or KCT8103L FEM.
    radio_driver.setTxPower(LORA_TX_POWER);
  }
  return initialized;
}

mesh::LocalIdentity radio_new_identity()
{
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);
}
