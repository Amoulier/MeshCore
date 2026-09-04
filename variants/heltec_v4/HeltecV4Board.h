#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>
#include <helpers/RefCountedDigitalPin.h>
#include "HeltecV4PowerPolicy.h"
#include "LoRaFEMControl.h"

#ifndef ADC_MULTIPLIER
#define ADC_MULTIPLIER (4.9f * 1.045f)
#endif

#ifndef BATTERY_PERCENT_SLEW_INTERVAL_MSEC
#define BATTERY_PERCENT_SLEW_INTERVAL_MSEC 60000UL
#endif

class HeltecV4Board : public ESP32Board {
  KeyValueStore *_prefs = NULL;
  float adc_mult = ADC_MULTIPLIER;
  uint16_t last_battery_millivolts = 0;
  int16_t reported_battery_percent = -1;
  uint32_t last_battery_percent_change = 0;
  uint32_t last_critical_battery_check = 0;
  uint8_t critical_low_readings = 0;
  int8_t last_requested_radio_dbm = 0;
  int8_t last_radio_input_dbm = 0;

  uint16_t readBatteryMilliVoltsRaw();
  void updateReportedBatteryPercent(uint16_t battery_millivolts);
  void configureCpuPowerManagement();
  void enterCriticalBatterySleep(bool runtime_shutdown);
  void releaseCriticalBatteryHolds();
  bool setLoRaFemLnaEnabled(bool enable);
  bool isLoRaFemLnaEnabled() const;

public:
  RefCountedDigitalPin periph_power;
  LoRaFEMControl loRaFEMControl;

  HeltecV4Board() : periph_power(PIN_VEXT_EN, PIN_VEXT_EN_ACTIVE) {}

  void begin();
  void loop() override;
  void attachDynamicPrefs(KeyValueStore *prefs);
  bool handleCommand(const char *command, uint32_t sender_timestamp, char *reply) override;
  bool startOTAUpdate(const char *id, char reply[]) override;

  void onBeforeTransmit() override;
  void onAfterTransmit() override;
  void powerOff() override;
  uint16_t getBattMilliVolts() override;
  int8_t getBattPercent() override;
  int8_t mapRadioTxPower(int8_t requested_radio_dbm) override;

  bool setAdcMultiplier(float multiplier) override
  {
    adc_mult = multiplier == 0.0f ? ADC_MULTIPLIER : multiplier;
    return true;
  }

  float getAdcMultiplier() const override { return adc_mult; }
  const char *getManufacturerName() const override;
};
