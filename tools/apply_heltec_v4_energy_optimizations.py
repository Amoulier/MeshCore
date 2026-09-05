#!/usr/bin/env python3
# Apply the Heltec V4 energy-efficiency implementation deterministically.
#
# This script is intentionally repository-local so the full transformation can
# be reviewed, reproduced and re-applied when rebasing the Heltec-only fork.

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel: str, text: str) -> None:
    path = ROOT / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def replace_once(rel: str, old: str, new: str) -> None:
    text = read(rel)
    if old not in text:
        if new in text:
            return
        raise RuntimeError(f"{rel}: expected replacement anchor not found:\n{old[:200]}")
    if text.count(old) != 1:
        raise RuntimeError(f"{rel}: replacement anchor occurs {text.count(old)} times")
    write(rel, text.replace(old, new, 1))


def regex_once(rel: str, pattern: str, repl: str, flags: int = 0) -> None:
    text = read(rel)
    updated, count = re.subn(pattern, repl, text, count=1, flags=flags)
    if count != 1:
        raise RuntimeError(f"{rel}: regex replacement count was {count}: {pattern}")
    write(rel, updated)


# ---------------------------------------------------------------------------
# PlatformIO targets: role-specific dependencies, PM profiles and variants.
# ---------------------------------------------------------------------------

rel = "variants/heltec_v4/platformio.ini"
text = read(rel)
text = text.replace(
    "  ${esp32_base.build_flags}\n  ${sensor_base.build_flags}\n",
    "  ${esp32_base.build_flags}\n",
    1,
)
text = text.replace(
    "  -D HELTEC_V4_SKIP_GPS_STARTUP_PROBE=1\n",
    "  -D HELTEC_V4_SKIP_GPS_STARTUP_PROBE=1\n"
    "  -D HELTEC_V4_BATTERY_CACHE_MSEC=10000\n"
    "  -D HELTEC_V4_TX_PA_SETTLE_US=20\n"
    "  -D NOISE_FLOOR_CALIB_FAST_INTERVAL=2000\n"
    "  -D NOISE_FLOOR_CALIB_STABLE_INTERVAL=30000\n"
    "  -D NOISE_FLOOR_CALIB_FAST_DURATION=60000\n",
    1,
)
text = text.replace("  -D LORA_TX_POWER=10\n", "  -D LORA_TX_POWER=11\n", 1)
text = text.replace(
    "lib_deps =\n  ${esp32_base.lib_deps}\n  ${sensor_base.lib_deps}\n\n[heltec_v4_oled]",
    "lib_deps =\n  ${esp32_base.lib_deps}\n\n"
    "[heltec_v4_sensor_drivers]\n"
    "build_flags =\n"
    "  ${sensor_base.build_flags}\n"
    "  -D ENV_HAS_I2C_SENSORS=1\n"
    "lib_deps =\n"
    "  ${sensor_base.lib_deps}\n\n"
    "[heltec_v4_oled]",
    1,
)

text = text.replace(
    "[heltec_v4_repeater_common]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  -D DISPLAY_CLASS=SSD1306Display\n",
    "[heltec_v4_repeater_common]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n"
    "  -D DISPLAY_CLASS=SSD1306Display\n",
    1,
)

text = text.replace(
    "[env:heltec_v4_solar_repeater]\n"
    "extends = heltec_v4_repeater_common\n"
    "build_unflags =\n"
    "  -D HELTEC_V4_DISPLAY_DEFAULT_OFF=0\n"
    "build_flags =\n"
    "  ${heltec_v4_repeater_common.build_flags}\n"
    "  -D HELTEC_V4_SOLAR_PROFILE=1\n"
    "  -D HELTEC_V4_DISPLAY_DEFAULT_OFF=1\n",
    "[env:heltec_v4_solar_repeater]\n"
    "extends = heltec_v4_repeater_common\n"
    "build_unflags =\n"
    "  -D HELTEC_V4_DISPLAY_DEFAULT_OFF=0\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n"
    "  -DBOARD_HAS_PSRAM\n"
    "build_flags =\n"
    "  ${heltec_v4_repeater_common.build_flags}\n"
    "  -D HELTEC_V4_SOLAR_PROFILE=1\n"
    "  -D HELTEC_V4_DISPLAY_DEFAULT_OFF=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=15\n"
    "  -D DEFAULT_POWERSAVING_ENABLED=1\n"
    "  -D POWERSAVING_FIRSTSLEEP_SECS=30\n\n"
    "[env:heltec_v4_solar_repeater_headless]\n"
    "extends = heltec_v4_solar_repeater\n"
    "build_unflags =\n"
    "  ${heltec_v4_solar_repeater.build_unflags}\n"
    "  -DARDUINO_USB_CDC_ON_BOOT=1\n"
    "  -DARDUINO_USB_MODE=1\n"
    "build_flags =\n"
    "  ${heltec_v4_solar_repeater.build_flags}\n"
    "  -DARDUINO_USB_CDC_ON_BOOT=0\n"
    "  -DARDUINO_USB_MODE=0\n"
    "  -D HELTEC_V4_HEADLESS=1\n",
    1,
)

text = text.replace(
    "[env:heltec_v4_expansionkit_repeater]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n",
    "[env:heltec_v4_expansionkit_repeater]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  ${heltec_v4_sensor_drivers.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_EXTERNAL_RTC_PROBE=1\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n",
    1,
)
text = text.replace(
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  ${esp32_ota.lib_deps}\n"
    "  bakercp/CRC32 @ ^2.0.0\n\n"
    "[env:heltec_v4_repeater_bridge_espnow]",
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  ${heltec_v4_sensor_drivers.lib_deps}\n"
    "  ${esp32_ota.lib_deps}\n"
    "  bakercp/CRC32 @ ^2.0.0\n\n"
    "[env:heltec_v4_repeater_bridge_espnow]",
    1,
)

text = text.replace(
    "[env:heltec_v4_repeater_bridge_espnow]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n",
    "[env:heltec_v4_repeater_bridge_espnow]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n",
    1,
)
text = text.replace(
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  ${esp32_ota.lib_deps}\n\n"
    "[env:heltec_v4_room_server]",
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  ${esp32_ota.lib_deps}\n\n"
    "[env:heltec_v4_repeater_bridge_espnow_low_power]\n"
    "extends = heltec_v4_repeater_bridge_espnow\n"
    "build_flags =\n"
    "  ${heltec_v4_repeater_bridge_espnow.build_flags}\n"
    "  -D HELTEC_V4_ESPNOW_LOW_POWER=1\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D HELTEC_V4_ESPNOW_WAKE_WINDOW_MS=20\n"
    "  -D HELTEC_V4_ESPNOW_WAKE_INTERVAL_MS=100\n\n"
    "[env:heltec_v4_room_server]",
    1,
)

text = text.replace(
    "[env:heltec_v4_room_server]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n",
    "[env:heltec_v4_room_server]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n",
    1,
)

text = text.replace(
    "[env:heltec_v4_companion_radio_ble]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n",
    "[env:heltec_v4_companion_radio_ble]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D HELTEC_V4_BLE_POWER_SAVE=1\n",
    1,
)
text = text.replace("  -D BLE_DEBUG_LOGGING=1\n", "", 1)
text = text.replace(
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  densaugeo/base64 @ ~1.4.0\n\n"
    "[env:heltec_v4_companion_radio_wifi]",
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  densaugeo/base64 @ ~1.4.0\n\n"
    "[env:heltec_v4_companion_radio_ble_low_power]\n"
    "extends = heltec_v4_companion_radio_ble\n"
    "build_flags =\n"
    "  ${heltec_v4_companion_radio_ble.build_flags}\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n\n"
    "[env:heltec_v4_companion_radio_wifi]",
    1,
)

text = text.replace(
    "[env:heltec_v4_companion_radio_wifi]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n",
    "[env:heltec_v4_companion_radio_wifi]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D HELTEC_V4_WIFI_POWER_SAVE=1\n",
    1,
)
text = text.replace("  -D WIFI_DEBUG_LOGGING=1\n", "", 1)
text = text.replace(
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  densaugeo/base64 @ ~1.4.0\n\n"
    "[env:heltec_v4_sensor]",
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  densaugeo/base64 @ ~1.4.0\n\n"
    "[env:heltec_v4_companion_radio_wifi_low_power]\n"
    "extends = heltec_v4_companion_radio_wifi\n"
    "build_flags =\n"
    "  ${heltec_v4_companion_radio_wifi.build_flags}\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D HELTEC_V4_WIFI_AGGRESSIVE_BACKOFF=1\n\n"
    "[env:heltec_v4_sensor]",
    1,
)

text = text.replace(
    "[env:heltec_v4_sensor]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n",
    "[env:heltec_v4_sensor]\n"
    "extends = heltec_v4_oled\n"
    "build_flags =\n"
    "  ${heltec_v4_oled.build_flags}\n"
    "  ${heltec_v4_sensor_drivers.build_flags}\n"
    "  -D HELTEC_V4_ENABLE_EXTERNAL_RTC_PROBE=1\n"
    "  -D HELTEC_V4_ENABLE_DFS=1\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n",
    1,
)
text = text.replace(
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  ${esp32_ota.lib_deps}\n\n"
    "[env:heltec_v4_kiss_modem]",
    "lib_deps =\n"
    "  ${heltec_v4_oled.lib_deps}\n"
    "  ${heltec_v4_sensor_drivers.lib_deps}\n"
    "  ${esp32_ota.lib_deps}\n\n"
    "[env:heltec_v4_sensor_low_power]\n"
    "extends = heltec_v4_sensor\n"
    "build_unflags =\n"
    "  -D HELTEC_V4_KEEP_ACCESSORY_RAIL_ON=1\n"
    "  -D HELTEC_V4_DISPLAY_DEFAULT_OFF=0\n"
    "  -D HELTEC_V4_AUTO_LIGHT_SLEEP=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=8\n"
    "  -D ENABLE_ADVERT_ON_BOOT=1\n"
    "  -DBOARD_HAS_PSRAM\n"
    "build_flags =\n"
    "  ${heltec_v4_sensor.build_flags}\n"
    "  -D HELTEC_V4_SENSOR_LOW_POWER=1\n"
    "  -D HELTEC_V4_DISPLAY_DEFAULT_OFF=1\n"
    "  -D DEFAULT_ADVERT_INTERVAL=0\n"
    "  -D ENABLE_ADVERT_ON_BOOT=0\n"
    "  -D SENSOR_READ_INTERVAL_SECS=1\n"
    "  -D HELTEC_V4_SENSOR_SLEEP_SECS=900\n"
    "  -D HELTEC_V4_SENSOR_AWAKE_MSEC=12000\n\n"
    "[env:heltec_v4_kiss_modem]",
    1,
)
write(rel, text)


# ---------------------------------------------------------------------------
# Heltec board: battery cache, PM, clean deep sleep, RX profiles and TX settle.
# ---------------------------------------------------------------------------

write(
    "variants/heltec_v4/HeltecV4Board.h",
    r'''#pragma once

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
  uint32_t last_battery_sample_millis = 0;
  int16_t reported_battery_percent = -1;
  uint32_t last_battery_percent_change = 0;
  uint32_t last_critical_battery_check = 0;
  uint8_t critical_low_readings = 0;
  int8_t last_requested_radio_dbm = 0;
  int8_t last_radio_input_dbm = 0;

  uint16_t readBatteryMilliVoltsRaw();
  void updateReportedBatteryPercent(uint16_t battery_millivolts);
  void configureCpuPowerManagement();
  void enterCriticalBatterySleep(bool runtime_shutdown, bool force_radio_reset);
  void releaseCriticalBatteryHolds();
  bool setLoRaFemLnaEnabled(bool enable);
  bool isLoRaFemLnaEnabled() const;

public:
  RefCountedDigitalPin periph_power;
  LoRaFEMControl loRaFEMControl;

  HeltecV4Board() : periph_power(PIN_VEXT_EN, PIN_VEXT_EN_ACTIVE) {}

  void begin();
  void loop() override;
  void idle(uint32_t delay_millis = 1);
  void enterDeepSleep(uint32_t secs);
  void attachDynamicPrefs(KeyValueStore *prefs);
  bool handleCommand(const char *command, uint32_t sender_timestamp, char *reply) override;
  bool startOTAUpdate(const char *id, char reply[]) override;

  void onBeforeTransmit() override;
  void onAfterTransmit() override;
  void powerOff() override;
  uint16_t getBattMilliVolts() override;
  uint16_t getBattMilliVoltsFresh();
  int8_t getBattPercent() override;
  int8_t mapRadioTxPower(int8_t requested_radio_dbm) override;

  bool setAdcMultiplier(float multiplier) override
  {
    adc_mult = multiplier == 0.0f ? ADC_MULTIPLIER : multiplier;
    last_battery_sample_millis = 0;
    return true;
  }

  float getAdcMultiplier() const override { return adc_mult; }
  const char *getManufacturerName() const override;
};
''',
)

replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "#ifndef HELTEC_V4_MAX_OUTPUT_POWER_DBM\n#define HELTEC_V4_MAX_OUTPUT_POWER_DBM 22\n#endif\n",
    "#ifndef HELTEC_V4_MAX_OUTPUT_POWER_DBM\n#define HELTEC_V4_MAX_OUTPUT_POWER_DBM 22\n#endif\n"
    "#ifndef HELTEC_V4_BATTERY_CACHE_MSEC\n#define HELTEC_V4_BATTERY_CACHE_MSEC 10000UL\n#endif\n"
    "#ifndef HELTEC_V4_TX_PA_SETTLE_US\n#define HELTEC_V4_TX_PA_SETTLE_US 20U\n#endif\n"
    "#ifndef HELTEC_V4_CPU_MAX_MHZ\n#define HELTEC_V4_CPU_MAX_MHZ 80\n#endif\n"
    "#ifndef HELTEC_V4_CPU_MIN_MHZ\n#define HELTEC_V4_CPU_MIN_MHZ 40\n#endif\n",
)

replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "__attribute__((noinline)) void heltecV4CriticalPreSleep() __attribute__((weak));\n"
    "__attribute__((noinline)) void heltecV4CriticalPreSleep() {}\n",
    "__attribute__((noinline)) void heltecV4CriticalPreSleep() __attribute__((weak));\n"
    "__attribute__((noinline)) void heltecV4CriticalPreSleep() {}\n\n"
    "__attribute__((noinline)) bool heltecV4SetInternalRxBoosted(bool enabled) __attribute__((weak));\n"
    "__attribute__((noinline)) bool heltecV4SetInternalRxBoosted(bool enabled)\n"
    "{\n"
    "  (void)enabled;\n"
    "  return false;\n"
    "}\n\n"
    "__attribute__((noinline)) int8_t heltecV4GetInternalRxBoosted() __attribute__((weak));\n"
    "__attribute__((noinline)) int8_t heltecV4GetInternalRxBoosted()\n"
    "{\n"
    "  return -1;\n"
    "}\n",
)

regex_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    r"void HeltecV4Board::configureCpuPowerManagement\(\)\n\{.*?\n\}\n\nvoid HeltecV4Board::enterCriticalBatterySleep",
    r'''void HeltecV4Board::configureCpuPowerManagement()
{
#if defined(HELTEC_V4_ENABLE_DFS) && HELTEC_V4_ENABLE_DFS && defined(CONFIG_PM_ENABLE) && CONFIG_PM_ENABLE
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_pm_config_t config = {};
#else
  esp_pm_config_esp32s3_t config = {};
#endif
  config.max_freq_mhz = HELTEC_V4_CPU_MAX_MHZ;
  config.min_freq_mhz = HELTEC_V4_CPU_MIN_MHZ;
#if defined(HELTEC_V4_AUTO_LIGHT_SLEEP) && HELTEC_V4_AUTO_LIGHT_SLEEP
  config.light_sleep_enable = true;
#else
  config.light_sleep_enable = false;
#endif
  const esp_err_t result = esp_pm_configure(&config);
  MESH_DEBUG_PRINTLN("Heltec V4 PM result=%d range=%d-%dMHz light_sleep=%d",
                     result, HELTEC_V4_CPU_MIN_MHZ, HELTEC_V4_CPU_MAX_MHZ,
                     config.light_sleep_enable ? 1 : 0);
#else
  MESH_DEBUG_PRINTLN("Heltec V4 DFS unavailable; retaining fixed %d MHz", HELTEC_V4_CPU_MAX_MHZ);
#endif
}

void HeltecV4Board::enterCriticalBatterySleep''',
    flags=re.S,
)

replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  const uint16_t boot_millivolts = readBatteryMilliVoltsRaw();\n",
    "  const uint16_t boot_millivolts = getBattMilliVoltsFresh();\n",
)
replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  periph_power.begin();\n  configureCpuPowerManagement();\n",
    "  periph_power.begin();\n"
    "#if defined(HELTEC_V4_KEEP_ACCESSORY_RAIL_ON) && HELTEC_V4_KEEP_ACCESSORY_RAIL_ON\n"
    "  // The board, not the OLED driver, owns the permanent accessory claim.\n"
    "  periph_power.claim();\n"
    "#endif\n"
    "  configureCpuPowerManagement();\n",
)
replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  const uint16_t battery_millivolts = getBattMilliVolts();\n"
    "  critical_low_readings = heltec_v4::updateLowReadingCounter(\n",
    "  const uint16_t battery_millivolts = getBattMilliVoltsFresh();\n"
    "  critical_low_readings = heltec_v4::updateLowReadingCounter(\n",
)
replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  loRaFEMControl.setTxModeEnable();\n}\n\nvoid HeltecV4Board::onAfterTransmit()",
    "  loRaFEMControl.setTxModeEnable();\n"
    "#if HELTEC_V4_TX_PA_SETTLE_US > 0\n"
    "  // Let the detected external FEM reach its TX state before the first preamble symbol.\n"
    "  delayMicroseconds(HELTEC_V4_TX_PA_SETTLE_US);\n"
    "#endif\n"
    "}\n\nvoid HeltecV4Board::onAfterTransmit()",
)
replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  const uint16_t battery_millivolts = readBatteryMilliVoltsRaw();\n"
    "  if (battery_millivolts < HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS) {\n",
    "  const uint16_t battery_millivolts = getBattMilliVoltsFresh();\n"
    "  if (battery_millivolts < HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS) {\n",
)

regex_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    r"void HeltecV4Board::powerOff\(\)\n\{.*?\n\}\n\nuint16_t HeltecV4Board::getBattMilliVolts\(\)\n\{.*?\n\}\n",
    r'''void HeltecV4Board::enterDeepSleep(uint32_t secs)
{
  persistent_writes_allowed = false;
  heltecV4CriticalPreSleep();
  loRaFEMControl.setSleepModeEnable();
  configureAndHoldPin(PIN_ADC_CTRL, LOW);
#ifdef PIN_OLED_RESET
  configureAndHoldPin(PIN_OLED_RESET, LOW);
#endif
  configureAndHoldPin(PIN_VEXT_EN, HIGH);
#ifdef PIN_GPS_EN
  configureAndHoldPin(PIN_GPS_EN, !PIN_GPS_EN_ACTIVE);
#endif
#ifdef PIN_GPS_RESET
  configureAndHoldPin(PIN_GPS_RESET, PIN_GPS_RESET_ACTIVE);
#endif
#ifdef P_LORA_TX_LED
  configureAndHoldPin(P_LORA_TX_LED, LOW);
#endif
  configureAndHoldPin(P_LORA_NSS, HIGH);
  configureAndHoldPin(P_LORA_KCT8103L_PA_CTX, HIGH);
  configureAndHoldPin(P_LORA_GC1109_PA_TX_EN, LOW);
  configureAndHoldPin(P_LORA_GC1109_PA_EN, LOW);
  configureAndHoldPin(P_LORA_PA_POWER, LOW);
  gpio_deep_sleep_hold_en();

  Serial.flush();
  delay(20);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  if (secs > 0) {
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(secs) * 1000000ULL);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
  }
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
}

void HeltecV4Board::powerOff()
{
  enterDeepSleep(0);
}

uint16_t HeltecV4Board::getBattMilliVoltsFresh()
{
  last_battery_millivolts = readBatteryMilliVoltsRaw();
  last_battery_sample_millis = millis();
  updateReportedBatteryPercent(last_battery_millivolts);
  return last_battery_millivolts;
}

uint16_t HeltecV4Board::getBattMilliVolts()
{
  const uint32_t now = millis();
  if (last_battery_millivolts != 0 &&
      static_cast<uint32_t>(now - last_battery_sample_millis) < HELTEC_V4_BATTERY_CACHE_MSEC) {
    return last_battery_millivolts;
  }
  return getBattMilliVoltsFresh();
}
''',
    flags=re.S,
)

replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "int8_t HeltecV4Board::getBattPercent()\n"
    "{\n"
    "  if (reported_battery_percent < 0) {\n"
    "    getBattMilliVolts();\n"
    "  }\n"
    "  return static_cast<int8_t>(reported_battery_percent);\n"
    "}\n",
    "int8_t HeltecV4Board::getBattPercent()\n"
    "{\n"
    "  if (reported_battery_percent < 0) {\n"
    "    getBattMilliVolts();\n"
    "  }\n"
    "  return static_cast<int8_t>(reported_battery_percent);\n"
    "}\n\n"
    "void HeltecV4Board::idle(uint32_t delay_millis)\n"
    "{\n"
    "  // Arduino delay yields to the FreeRTOS idle task, allowing DFS and\n"
    "  // automatic light sleep where the selected target enables them.\n"
    "  delay(delay_millis == 0 ? 1 : delay_millis);\n"
    "}\n",
)

replace_once(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  if (strcmp(command, \"get radio.power\") == 0) {\n",
    "  if (strcmp(command, \"get radio.rxprofile\") == 0) {\n"
    "    const int8_t internal = heltecV4GetInternalRxBoosted();\n"
    "    const bool external_supported = loRaFEMControl.isLnaCanControl();\n"
    "    const bool external = !external_supported || isLoRaFemLnaEnabled();\n"
    "    const char *profile = \"custom\";\n"
    "    if (internal > 0 && external) {\n"
    "      profile = \"range\";\n"
    "    } else if (internal == 0 && external) {\n"
    "      profile = \"balanced\";\n"
    "    } else if (internal == 0 && !external) {\n"
    "      profile = \"battery\";\n"
    "    }\n"
    "    sprintf(reply, \"> %s (sx1262:%s,fem:%s)\", profile,\n"
    "            internal > 0 ? \"boosted\" : (internal == 0 ? \"normal\" : \"unknown\"),\n"
    "            external_supported ? (external ? \"lna\" : \"bypass\") : \"fixed\");\n"
    "    return true;\n"
    "  }\n\n"
    "  if (memcmp(command, \"set radio.rxprofile \", 20) == 0) {\n"
    "    const char *value = &command[20];\n"
    "    const bool range = strcmp(value, \"range\") == 0;\n"
    "    const bool balanced = strcmp(value, \"balanced\") == 0;\n"
    "    const bool battery = strcmp(value, \"battery\") == 0;\n"
    "    if (!range && !balanced && !battery) {\n"
    "      strcpy(reply, \"Error: profile must be range, balanced or battery\");\n"
    "      return true;\n"
    "    }\n"
    "    const bool internal_boost = range;\n"
    "    const bool external_lna = range || balanced;\n"
    "    if (!heltecV4SetInternalRxBoosted(internal_boost)) {\n"
    "      strcpy(reply, \"Error: failed to set SX1262 RX gain\");\n"
    "      return true;\n"
    "    }\n"
    "    if (loRaFEMControl.isLnaCanControl() && !setLoRaFemLnaEnabled(external_lna)) {\n"
    "      strcpy(reply, \"Error: failed to set FEM RX gain\");\n"
    "      return true;\n"
    "    }\n"
    "    if (_prefs) {\n"
    "      _prefs->setByKey(\"rxgain\", internal_boost ? \"1\" : \"0\");\n"
    "      _prefs->setByKey(\"fem_rxgain\", external_lna ? \"1\" : \"0\");\n"
    "    }\n"
    "    fem_lna_nvs_explicit = persistFemLnaPreference(external_lna) || fem_lna_nvs_explicit;\n"
    "    sprintf(reply, \"OK - RX profile %s\", value);\n"
    "    return true;\n"
    "  }\n\n"
    "  if (strcmp(command, \"get radio.power\") == 0) {\n",
)


# Target hooks and optional external RTC discovery.
replace_once(
    "variants/heltec_v4/target.cpp",
    "WRAPPER_CLASS radio_driver(radio, board);\n\nESP32RTCClock fallback_clock;",
    "WRAPPER_CLASS radio_driver(radio, board);\n\n"
    "bool heltecV4SetInternalRxBoosted(bool enabled)\n"
    "{\n"
    "  return radio_driver.setRxBoostedGainMode(enabled);\n"
    "}\n\n"
    "int8_t heltecV4GetInternalRxBoosted()\n"
    "{\n"
    "  return radio_driver.getRxBoostedGainMode() ? 1 : 0;\n"
    "}\n\n"
    "ESP32RTCClock fallback_clock;",
)
replace_once(
    "variants/heltec_v4/target.cpp",
    "  fallback_clock.begin();\n  rtc_clock.begin(Wire);\n",
    "  fallback_clock.begin();\n"
    "#if defined(HELTEC_V4_ENABLE_EXTERNAL_RTC_PROBE) && HELTEC_V4_ENABLE_EXTERNAL_RTC_PROBE\n"
    "  rtc_clock.begin(Wire);\n"
    "#endif\n",
)


# OLED owns only its own VEXT claim; accessory roles are owned by the board.
rel = "src/helpers/ui/SSD1306Display.cpp"
text = read(rel)
text = re.sub(
    r"\nconstexpr bool keepAccessoryRailPowered\(\)\n\{.*?\n\}\n",
    "\n",
    text,
    count=1,
    flags=re.S,
)
text = text.replace(" && !keepAccessoryRailPowered()", "")
text = text.replace(
    "  if (_persistentlyDisabled) {\n"
    "    if (_peripher_power && !_railClaimed && keepAccessoryRailPowered()) {\n"
    "      _peripher_power->claim();\n"
    "      _railClaimed = true;\n"
    "    }\n"
    "    powerDownPanel();\n"
    "    return true;\n"
    "  }\n",
    "  if (_persistentlyDisabled) {\n"
    "    powerDownPanel();\n"
    "    return true;\n"
    "  }\n",
    1,
)
write(rel, text)


# Sensor discovery is compiled only for roles that explicitly include drivers.
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "#include <Wire.h>\n",
    "#include <Wire.h>\n#include <helpers/PersistentWriteGuard.h>\n",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "static void scanI2CBus(TwoWire* wire, bool found[128]) {\n",
    "#if defined(ENV_HAS_I2C_SENSORS) && ENV_HAS_I2C_SENSORS\n"
    "static void scanI2CBus(TwoWire* wire, bool found[128]) {\n",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "}\n\n// ============================================================\n// Per-sensor init and query functions",
    "}\n"
    "#endif\n\n"
    "// ============================================================\n// Per-sensor init and query functions",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "  #if ENV_PIN_SDA && ENV_PIN_SCL\n",
    "#if defined(ENV_HAS_I2C_SENSORS) && ENV_HAS_I2C_SENSORS\n"
    "  #if ENV_PIN_SDA && ENV_PIN_SCL\n",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "  return true;\n}\n\n// ============================================================\n// querySensors()",
    "#else\n"
    "  _active_sensor_count = 0;\n"
    "#endif\n\n"
    "  return true;\n"
    "}\n\n"
    "// ============================================================\n// querySensors()",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "  gps_detected = true;\n  _location->stop();\n  gps_active = false;\n  return;\n",
    "  gps_detected = true;\n"
    "  _location->stop();\n"
    "  Serial1.end();\n"
    "  gps_active = false;\n"
    "  return;\n",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "  _location->begin();\n  _location->reset();\n\n#ifndef PIN_GPS_EN",
    "  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);\n"
    "#ifdef GPS_BAUD_RATE\n"
    "  Serial1.begin(GPS_BAUD_RATE);\n"
    "#else\n"
    "  Serial1.begin(9600);\n"
    "#endif\n"
    "  _location->begin();\n"
    "  _location->reset();\n\n"
    "#ifndef PIN_GPS_EN",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "  _location->stop();\n\n  #ifndef PIN_GPS_EN",
    "  _location->stop();\n"
    "  Serial1.end();\n\n"
    "  #ifndef PIN_GPS_EN",
)
replace_once(
    "src/helpers/sensors/EnvironmentSensorManager.cpp",
    "static void bsec_save_state() {\n",
    "static void bsec_save_state() {\n"
    "  if (!meshcorePersistentWritesAllowed()) return;\n",
)


# Dispatcher: fast startup calibration followed by a low-overhead stable cadence.
replace_once(
    "src/Dispatcher.h",
    "  unsigned long next_floor_calib_time, next_agc_reset_time;\n",
    "  unsigned long next_floor_calib_time, next_agc_reset_time;\n"
    "  unsigned long floor_calib_started_at;\n",
)
replace_once(
    "src/Dispatcher.h",
    "    next_floor_calib_time = next_agc_reset_time = 0;\n",
    "    next_floor_calib_time = next_agc_reset_time = 0;\n"
    "    floor_calib_started_at = ms.getMillis();\n",
)
replace_once(
    "src/Dispatcher.cpp",
    "#ifndef NOISE_FLOOR_CALIB_INTERVAL\n"
    "  #define NOISE_FLOOR_CALIB_INTERVAL   2000     // 2 seconds\n"
    "#endif\n",
    "#ifndef NOISE_FLOOR_CALIB_FAST_INTERVAL\n"
    "  #define NOISE_FLOOR_CALIB_FAST_INTERVAL 2000UL\n"
    "#endif\n"
    "#ifndef NOISE_FLOOR_CALIB_STABLE_INTERVAL\n"
    "  #define NOISE_FLOOR_CALIB_STABLE_INTERVAL 30000UL\n"
    "#endif\n"
    "#ifndef NOISE_FLOOR_CALIB_FAST_DURATION\n"
    "  #define NOISE_FLOOR_CALIB_FAST_DURATION 60000UL\n"
    "#endif\n",
)
replace_once(
    "src/Dispatcher.cpp",
    "  radio_nonrx_start = _ms->getMillis();\n",
    "  radio_nonrx_start = _ms->getMillis();\n"
    "  floor_calib_started_at = radio_nonrx_start;\n",
)
replace_once(
    "src/Dispatcher.cpp",
    "  if (millisHasNowPassed(next_floor_calib_time)) {\n"
    "    _radio->triggerNoiseFloorCalibrate(getInterferenceThreshold());\n"
    "    _radio->setCADEnabled(getCADEnabled());\n"
    "    next_floor_calib_time = futureMillis(NOISE_FLOOR_CALIB_INTERVAL);\n"
    "  }\n",
    "  if (millisHasNowPassed(next_floor_calib_time)) {\n"
    "    _radio->triggerNoiseFloorCalibrate(getInterferenceThreshold());\n"
    "    _radio->setCADEnabled(getCADEnabled());\n"
    "    const unsigned long age = _ms->getMillis() - floor_calib_started_at;\n"
    "    const unsigned long interval = age < NOISE_FLOOR_CALIB_FAST_DURATION\n"
    "        ? NOISE_FLOOR_CALIB_FAST_INTERVAL\n"
    "        : NOISE_FLOOR_CALIB_STABLE_INTERVAL;\n"
    "    next_floor_calib_time = futureMillis(interval);\n"
    "  }\n",
)

replace_once(
    "src/helpers/radiolib/RadioLibWrappers.cpp",
    "void RadioLibWrapper::loop() {\n  _board->loop();\n\n",
    "void RadioLibWrapper::loop() {\n",
)


# BLE: two-stage advertising and adaptive connection intervals.
write(
    "src/helpers/esp32/SerialBLEInterface.h",
    r'''#pragma once

#include "../BaseSerialInterface.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class SerialBLEInterface : public BaseSerialInterface, BLESecurityCallbacks, BLEServerCallbacks, BLECharacteristicCallbacks {
  BLEServer *pServer;
  BLEService *pService;
  BLECharacteristic *pTxCharacteristic;
  bool deviceConnected;
  bool oldDeviceConnected;
  bool _isEnabled;
  uint16_t last_conn_id;
  uint32_t _pin_code;
  unsigned long _last_write;
  unsigned long adv_restart_time;
  unsigned long fast_adv_until;
  unsigned long last_activity_time;
  bool advertising_fast;
  bool peer_address_valid;
  bool connection_profile_known;
  bool idle_connection_profile;
  uint8_t peer_address[ESP_BD_ADDR_LEN];

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define FRAME_QUEUE_SIZE 4
  StaticQueue_t recv_queue_state;
  uint8_t recv_queue_storage[FRAME_QUEUE_SIZE * sizeof(Frame)];
  QueueHandle_t recv_queue;
  int send_queue_len;
  Frame send_queue[FRAME_QUEUE_SIZE];

  void clearBuffers();
  void startAdvertising(bool fast);
  void markActivity();
  void requestConnectionProfile(bool idle);
  void updatePowerPolicy();

protected:
  uint32_t onPassKeyRequest() override;
  void onPassKeyNotify(uint32_t pass_key) override;
  bool onConfirmPIN(uint32_t pass_key) override;
  bool onSecurityRequest() override;
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;

  void onConnect(BLEServer *pServer) override;
  void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override;
  void onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override;
  void onDisconnect(BLEServer *pServer) override;

  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;

public:
  SerialBLEInterface()
  {
    pServer = NULL;
    pService = NULL;
    pTxCharacteristic = NULL;
    deviceConnected = false;
    oldDeviceConnected = false;
    adv_restart_time = 0;
    fast_adv_until = 0;
    last_activity_time = 0;
    advertising_fast = false;
    peer_address_valid = false;
    connection_profile_known = false;
    idle_connection_profile = false;
    _isEnabled = false;
    _last_write = 0;
    last_conn_id = 0;
    memset(peer_address, 0, sizeof(peer_address));
    recv_queue = xQueueCreateStatic(
        FRAME_QUEUE_SIZE, sizeof(Frame), recv_queue_storage, &recv_queue_state);
    send_queue_len = 0;
  }

  void begin(const char *prefix, char *name, uint32_t pin_code);

  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }
  bool isConnected() const override;
  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;
};

#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINT(F, ...) Serial.printf("BLE: " F, ##__VA_ARGS__)
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINT(...) {}
  #define BLE_DEBUG_PRINTLN(...) {}
#endif
''',
)

write(
    "src/helpers/esp32/SerialBLEInterface.cpp",
    r'''#include "SerialBLEInterface.h"
#include "esp_mac.h"
#include <esp_gap_ble_api.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ADVERT_RESTART_DELAY 1000UL
#define BLE_FAST_ADV_DURATION_MS 60000UL
#define BLE_IDLE_CONNECTION_AFTER_MS 10000UL

// Advertising units are 0.625 ms.
#define BLE_FAST_ADV_MIN 160
#define BLE_FAST_ADV_MAX 240
#define BLE_SLOW_ADV_MIN 800
#define BLE_SLOW_ADV_MAX 1600

void SerialBLEInterface::begin(const char *prefix, char *name, uint32_t pin_code)
{
  _pin_code = pin_code;

  if (strcmp(name, "@@MAC") == 0) {
    uint8_t addr[8] = {};
    esp_efuse_mac_get_default(addr);
    sprintf(name, "%02X%02X%02X%02X%02X%02X",
            addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
  }
  char dev_name[48];
  snprintf(dev_name, sizeof(dev_name), "%s%s", prefix, name);

  BLEDevice::init(dev_name);
  BLEDevice::setSecurityCallbacks(this);
  BLEDevice::setMTU(MAX_FRAME_SIZE);

  BLESecurity sec;
  sec.setStaticPIN(pin_code);
  sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);
  pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic =
      pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  pRxCharacteristic->setCallbacks(this);

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
}

void SerialBLEInterface::startAdvertising(bool fast)
{
  if (!_isEnabled || pServer == NULL) return;
  BLEAdvertising *advertising = pServer->getAdvertising();
  advertising->stop();
  advertising->setMinInterval(fast ? BLE_FAST_ADV_MIN : BLE_SLOW_ADV_MIN);
  advertising->setMaxInterval(fast ? BLE_FAST_ADV_MAX : BLE_SLOW_ADV_MAX);
  advertising->start();
  advertising_fast = fast;
  fast_adv_until = fast ? millis() + BLE_FAST_ADV_DURATION_MS : 0;
  adv_restart_time = 0;
}

void SerialBLEInterface::markActivity()
{
  last_activity_time = millis();
  if (deviceConnected && idle_connection_profile) {
    requestConnectionProfile(false);
  }
}

void SerialBLEInterface::requestConnectionProfile(bool idle)
{
#if defined(HELTEC_V4_BLE_POWER_SAVE) && HELTEC_V4_BLE_POWER_SAVE
  if (!deviceConnected || !peer_address_valid) return;
  if (connection_profile_known && idle_connection_profile == idle) return;

  esp_ble_conn_update_params_t params = {};
  memcpy(params.bda, peer_address, ESP_BD_ADDR_LEN);
  if (idle) {
    params.min_int = 48;
    params.max_int = 96;
    params.latency = 2;
    params.timeout = 500;
  } else {
    params.min_int = 12;
    params.max_int = 24;
    params.latency = 0;
    params.timeout = 400;
  }
  if (esp_ble_gap_update_conn_params(&params) == ESP_OK) {
    idle_connection_profile = idle;
    connection_profile_known = true;
  }
#else
  (void)idle;
#endif
}

void SerialBLEInterface::updatePowerPolicy()
{
  const unsigned long now = millis();
  if (!deviceConnected && advertising_fast && fast_adv_until &&
      static_cast<long>(now - fast_adv_until) >= 0) {
    startAdvertising(false);
  }
  if (deviceConnected && last_activity_time &&
      static_cast<uint32_t>(now - last_activity_time) >= BLE_IDLE_CONNECTION_AFTER_MS) {
    requestConnectionProfile(true);
  }
}

uint32_t SerialBLEInterface::onPassKeyRequest()
{
  BLE_DEBUG_PRINTLN("onPassKeyRequest()");
  return _pin_code;
}

void SerialBLEInterface::onPassKeyNotify(uint32_t pass_key)
{
  BLE_DEBUG_PRINTLN("onPassKeyNotify(%u)", pass_key);
}

bool SerialBLEInterface::onConfirmPIN(uint32_t pass_key)
{
  BLE_DEBUG_PRINTLN("onConfirmPIN(%u)", pass_key);
  return true;
}

bool SerialBLEInterface::onSecurityRequest()
{
  BLE_DEBUG_PRINTLN("onSecurityRequest()");
  return true;
}

void SerialBLEInterface::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl)
{
  if (cmpl.success) {
    BLE_DEBUG_PRINTLN("Authentication Success");
    deviceConnected = true;
    connection_profile_known = false;
    markActivity();
  } else {
    BLE_DEBUG_PRINTLN("Authentication Failure");
    pServer->disconnect(pServer->getConnId());
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  }
}

void SerialBLEInterface::onConnect(BLEServer *pServer)
{
  (void)pServer;
}

void SerialBLEInterface::onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  BLE_DEBUG_PRINTLN("onConnect(), conn_id=%d, mtu=%d",
                    param->connect.conn_id, pServer->getPeerMTU(param->connect.conn_id));
  last_conn_id = param->connect.conn_id;
  memcpy(peer_address, param->connect.remote_bda, ESP_BD_ADDR_LEN);
  peer_address_valid = true;
  connection_profile_known = false;
  last_activity_time = millis();
}

void SerialBLEInterface::onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  BLE_DEBUG_PRINTLN("onMtuChanged(), mtu=%d", pServer->getPeerMTU(param->mtu.conn_id));
  markActivity();
}

void SerialBLEInterface::onDisconnect(BLEServer *pServer)
{
  (void)pServer;
  BLE_DEBUG_PRINTLN("onDisconnect()");
  deviceConnected = false;
  peer_address_valid = false;
  connection_profile_known = false;
  if (_isEnabled) adv_restart_time = millis() + ADVERT_RESTART_DELAY;
}

void SerialBLEInterface::onWrite(BLECharacteristic *pCharacteristic,
                                 esp_ble_gatts_cb_param_t *param)
{
  (void)param;
  uint8_t *rxValue = pCharacteristic->getData();
  int len = pCharacteristic->getLength();

  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), frame too big, len=%d", len);
  } else {
    Frame frame = {};
    frame.len = len;
    memcpy(frame.buf, rxValue, len);
    if (xQueueSend(recv_queue, &frame, 0) != pdTRUE) {
      BLE_DEBUG_PRINTLN("ERROR: onWrite(), recv_queue is full!");
    } else {
      markActivity();
    }
  }
}

void SerialBLEInterface::clearBuffers()
{
  xQueueReset(recv_queue);
  send_queue_len = 0;
}

void SerialBLEInterface::enable()
{
  if (_isEnabled) return;
  _isEnabled = true;
  clearBuffers();
  pService->start();
  startAdvertising(true);
}

void SerialBLEInterface::disable()
{
  _isEnabled = false;
  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");
  pServer->getAdvertising()->stop();
  if (pServer->getConnectedCount() > 0) pServer->disconnect(last_conn_id);
  pService->stop();
  oldDeviceConnected = deviceConnected = false;
  peer_address_valid = false;
  connection_profile_known = false;
  adv_restart_time = fast_adv_until = 0;
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len)
{
  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("writeFrame(), frame too big, len=%d", len);
    return 0;
  }
  if (deviceConnected && len > 0) {
    if (send_queue_len >= FRAME_QUEUE_SIZE) {
      BLE_DEBUG_PRINTLN("writeFrame(), send_queue is full!");
      return 0;
    }
    send_queue[send_queue_len].len = len;
    memcpy(send_queue[send_queue_len].buf, src, len);
    send_queue_len++;
    markActivity();
    return len;
  }
  return 0;
}

#define BLE_WRITE_MIN_INTERVAL 60

bool SerialBLEInterface::isWriteBusy() const
{
  return millis() < _last_write + BLE_WRITE_MIN_INTERVAL;
}

size_t SerialBLEInterface::checkRecvFrame(uint8_t dest[])
{
  if (send_queue_len > 0 && millis() >= _last_write + BLE_WRITE_MIN_INTERVAL) {
    _last_write = millis();
    pTxCharacteristic->setValue(send_queue[0].buf, send_queue[0].len);
    pTxCharacteristic->notify();
    markActivity();

    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) send_queue[i] = send_queue[i + 1];
  }

  Frame frame;
  if (xQueueReceive(recv_queue, &frame, 0) == pdTRUE) {
    memcpy(dest, frame.buf, frame.len);
    markActivity();
    return frame.len;
  }

  if (deviceConnected != oldDeviceConnected) {
    if (!deviceConnected) {
      clearBuffers();
      adv_restart_time = millis() + ADVERT_RESTART_DELAY;
    } else {
      pServer->getAdvertising()->stop();
      adv_restart_time = fast_adv_until = 0;
      requestConnectionProfile(false);
    }
    oldDeviceConnected = deviceConnected;
  }

  if (adv_restart_time && static_cast<long>(millis() - adv_restart_time) >= 0) {
    if (pServer->getConnectedCount() == 0) startAdvertising(true);
    adv_restart_time = 0;
  }

  updatePowerPolicy();
  return 0;
}

bool SerialBLEInterface::isConnected() const
{
  return deviceConnected;
}
''',
)


# Wi-Fi modem sleep and exponential reconnect backoff.
replace_once(
    "examples/companion_radio/main.cpp",
    "#if defined(ESP32) && defined(WIFI_SSID)\n"
    "  bool wifi_needs_reconnect = false;\n"
    "  unsigned long last_wifi_reconnect_attempt = 0;\n"
    "#endif\n",
    "#if defined(ESP32) && defined(WIFI_SSID)\n"
    "  #include <esp_wifi.h>\n"
    "  bool wifi_needs_reconnect = false;\n"
    "  unsigned long last_wifi_reconnect_attempt = 0;\n"
    "  unsigned long wifi_reconnect_delay_ms = 10000;\n"
    "#endif\n",
)
replace_once(
    "examples/companion_radio/main.cpp",
    "void setup() {\n  Serial.begin(115200);\n  board.begin();\n",
    "void setup() {\n  board.begin();\n  Serial.begin(115200);\n",
)
replace_once(
    "examples/companion_radio/main.cpp",
    "          wifi_needs_reconnect = true;\n"
    "      } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {\n"
    "          WIFI_DEBUG_PRINTLN(\"WiFi connected successfully!\");\n"
    "          wifi_needs_reconnect = false;\n",
    "          wifi_needs_reconnect = true;\n"
    "      } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {\n"
    "          WIFI_DEBUG_PRINTLN(\"WiFi connected successfully!\");\n"
    "          wifi_needs_reconnect = false;\n"
    "          wifi_reconnect_delay_ms = 10000;\n",
)
replace_once(
    "examples/companion_radio/main.cpp",
    "  WiFi.begin(WIFI_SSID, WIFI_PWD);\n"
    "  wifi_interface.begin(TCP_PORT);\n",
    "  WiFi.setSleep(true);\n"
    "#if defined(HELTEC_V4_WIFI_POWER_SAVE) && HELTEC_V4_WIFI_POWER_SAVE\n"
    "  #if defined(HELTEC_V4_WIFI_AGGRESSIVE_BACKOFF) && HELTEC_V4_WIFI_AGGRESSIVE_BACKOFF\n"
    "  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);\n"
    "  #else\n"
    "  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);\n"
    "  #endif\n"
    "#endif\n"
    "  WiFi.begin(WIFI_SSID, WIFI_PWD);\n"
    "  wifi_interface.begin(TCP_PORT);\n",
)
replace_once(
    "examples/companion_radio/main.cpp",
    "  if (wifi_needs_reconnect && (millis() - last_wifi_reconnect_attempt > 10000)) {\n"
    "    WIFI_DEBUG_PRINTLN(\"Attempting manual WiFi reconnect...\");\n"
    "    WiFi.disconnect();\n"
    "    WiFi.reconnect();\n"
    "    last_wifi_reconnect_attempt = millis();\n"
    "  }\n"
    "#endif\n"
    "}\n",
    "  if (wifi_needs_reconnect &&\n"
    "      (millis() - last_wifi_reconnect_attempt > wifi_reconnect_delay_ms)) {\n"
    "    WIFI_DEBUG_PRINTLN(\"Attempting manual WiFi reconnect...\");\n"
    "    WiFi.disconnect();\n"
    "    WiFi.reconnect();\n"
    "    last_wifi_reconnect_attempt = millis();\n"
    "#if defined(HELTEC_V4_WIFI_AGGRESSIVE_BACKOFF) && HELTEC_V4_WIFI_AGGRESSIVE_BACKOFF\n"
    "    const unsigned long max_backoff = 900000;\n"
    "#else\n"
    "    const unsigned long max_backoff = 300000;\n"
    "#endif\n"
    "    wifi_reconnect_delay_ms = min(wifi_reconnect_delay_ms * 2UL, max_backoff);\n"
    "  }\n"
    "#endif\n"
    "\n"
    "  board.idle();\n"
    "}\n",
)


# ESP-NOW optional receive windows.
replace_once(
    "src/helpers/bridges/ESPNowBridge.cpp",
    "#include <esp_wifi.h>\n",
    "#include <esp_wifi.h>\n#include <esp_idf_version.h>\n",
)
replace_once(
    "src/helpers/bridges/ESPNowBridge.cpp",
    "  // Register callbacks\n",
    "#if defined(HELTEC_V4_ESPNOW_LOW_POWER) && HELTEC_V4_ESPNOW_LOW_POWER && \\\n"
    "    ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)\n"
    "  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);\n"
    "  esp_now_set_wake_window(HELTEC_V4_ESPNOW_WAKE_WINDOW_MS);\n"
    "  esp_wifi_connectionless_module_set_wake_interval(HELTEC_V4_ESPNOW_WAKE_INTERVAL_MS);\n"
    "#endif\n\n"
    "  // Register callbacks\n",
)


# Role loops: early board initialization, cooperative idle and solar sleep default.
replace_once(
    "examples/simple_repeater/main.cpp",
    "unsigned long POWERSAVING_FIRSTSLEEP_SECS = 120; // The first sleep (if enabled) from boot\n",
    "#ifndef POWERSAVING_FIRSTSLEEP_SECS\n"
    "#define POWERSAVING_FIRSTSLEEP_SECS 120\n"
    "#endif\n"
    "static constexpr unsigned long kPowerSavingFirstSleepSecs = POWERSAVING_FIRSTSLEEP_SECS;\n",
)
replace_once(
    "examples/simple_repeater/main.cpp",
    "void setup() {\n  Serial.begin(115200);\n  delay(1000);\n\n  board.begin();\n",
    "void setup() {\n  board.begin();\n  Serial.begin(115200);\n",
)
replace_once(
    "examples/simple_repeater/main.cpp",
    "    if (the_mesh.millisHasNowPassed(POWERSAVING_FIRSTSLEEP_SECS * 1000)) { // To check if it is time to sleep\n",
    "    if (the_mesh.millisHasNowPassed(kPowerSavingFirstSleepSecs * 1000)) { // To check if it is time to sleep\n",
)
text = read("examples/simple_repeater/main.cpp")
pos = text.rfind("\n}")
text = text[:pos] + "\n  board.idle();" + text[pos:]
write("examples/simple_repeater/main.cpp", text)

replace_once(
    "examples/simple_room_server/main.cpp",
    "void setup() {\n  Serial.begin(115200);\n  delay(1000);\n\n  board.begin();\n",
    "void setup() {\n  board.begin();\n  Serial.begin(115200);\n",
)
text = read("examples/simple_room_server/main.cpp")
pos = text.rfind("\n}")
text = text[:pos] + "\n  board.idle();" + text[pos:]
write("examples/simple_room_server/main.cpp", text)

replace_once(
    "examples/simple_sensor/main.cpp",
    "void setup() {\n  Serial.begin(115200);\n  delay(1000);\n\n  board.begin();\n",
    "void setup() {\n  board.begin();\n  Serial.begin(115200);\n"
    "#if defined(HELTEC_V4_SENSOR_LOW_POWER) && HELTEC_V4_SENSOR_LOW_POWER\n"
    "  // Keep the sensor rail up for this short wake cycle. Deep sleep forces it off.\n"
    "  board.periph_power.claim();\n"
    "#endif\n",
)
replace_once(
    "examples/simple_sensor/main.cpp",
    "#if ENABLE_ADVERT_ON_BOOT == 1\n"
    "  the_mesh.sendSelfAdvertisement(16000, false);\n"
    "#endif\n"
    "}\n",
    "#if ENABLE_ADVERT_ON_BOOT == 1\n"
    "  the_mesh.sendSelfAdvertisement(16000, false);\n"
    "#endif\n"
    "#if defined(HELTEC_V4_SENSOR_LOW_POWER) && HELTEC_V4_SENSOR_LOW_POWER\n"
    "  the_mesh.sendSelfAdvertisement(500, false);\n"
    "#endif\n"
    "}\n",
)
text = read("examples/simple_sensor/main.cpp")
pos = text.rfind("\n}")
low_power_tail = r'''
#if defined(HELTEC_V4_SENSOR_LOW_POWER) && HELTEC_V4_SENSOR_LOW_POWER
  if (millis() >= HELTEC_V4_SENSOR_AWAKE_MSEC && !the_mesh.hasPendingWork()) {
    board.enterDeepSleep(HELTEC_V4_SENSOR_SLEEP_SECS);
  }
#endif
  board.idle();'''
text = text[:pos] + "\n" + low_power_tail + text[pos:]
write("examples/simple_sensor/main.cpp", text)

replace_once(
    "examples/simple_sensor/SensorMesh.h",
    "  void handleCommand(uint32_t sender_timestamp, char* command, char* reply);\n",
    "  void handleCommand(uint32_t sender_timestamp, char* command, char* reply);\n"
    "  bool hasPendingWork() const {\n"
    "    return _mgr->getOutboundTotal() > 0 || dirty_contacts_expiry != 0 || num_alert_tasks > 0;\n"
    "  }\n",
)

replace_once(
    "examples/simple_secure_chat/main.cpp",
    "void setup() {\n  Serial.begin(115200);\n\n  board.begin();\n",
    "void setup() {\n  board.begin();\n  Serial.begin(115200);\n",
)
text = read("examples/simple_secure_chat/main.cpp")
pos = text.rfind("\n}")
text = text[:pos] + "\n  board.idle();" + text[pos:]
write("examples/simple_secure_chat/main.cpp", text)

text = read("examples/kiss_modem/main.cpp")
pos = text.rfind("\n}")
text = text[:pos] + "\n  board.idle();" + text[pos:]
write("examples/kiss_modem/main.cpp", text)


# Defaults: less frequent advertisements; Solar enables existing DIO1 light sleep.
for rel in (
    "examples/simple_repeater/MyMesh.cpp",
    "examples/simple_room_server/MyMesh.cpp",
    "examples/simple_sensor/SensorMesh.cpp",
):
    text = read(rel)
    if "#ifndef DEFAULT_ADVERT_INTERVAL" not in text:
        anchor = "#ifndef LORA_FREQ\n"
        if anchor not in text:
            raise RuntimeError(f"{rel}: config anchor missing")
        text = text.replace(
            anchor,
            "#ifndef DEFAULT_ADVERT_INTERVAL\n"
            "#define DEFAULT_ADVERT_INTERVAL 1\n"
            "#endif\n"
            "#ifndef DEFAULT_POWERSAVING_ENABLED\n"
            "#define DEFAULT_POWERSAVING_ENABLED 0\n"
            "#endif\n\n"
            + anchor,
            1,
        )
    text = text.replace(
        "_prefs.advert_interval = 1;",
        "_prefs.advert_interval = DEFAULT_ADVERT_INTERVAL;",
        1,
    )
    if rel == "examples/simple_repeater/MyMesh.cpp":
        marker = "  _prefs.adc_multiplier = 0.0f; // 0.0f means use default board multiplier\n"
        if marker not in text:
            raise RuntimeError(f"{rel}: power default anchor missing")
        text = text.replace(
            marker,
            marker + "  _prefs.powersaving_enabled = DEFAULT_POWERSAVING_ENABLED;\n",
            1,
        )
    write(rel, text)


# Power-policy tests cover the stronger safe GC1109 default.
replace_once(
    "test/test_heltec_v4_power_policy/test_heltec_v4_power_policy.cpp",
    "  EXPECT_EQ(21, gc1109EstimatedOutput(10));\n"
    "}\n",
    "  EXPECT_EQ(21, gc1109EstimatedOutput(10));\n"
    "  EXPECT_EQ(22, gc1109EstimatedOutput(clampGc1109RadioInput(11, 22)));\n"
    "  EXPECT_EQ(22, kct8103lEstimatedOutput(clampKct8103lRadioInput(11, 22)));\n"
    "}\n",
)


# ---------------------------------------------------------------------------
# Build/release matrices.
# ---------------------------------------------------------------------------

targets = [
    "heltec_v4_repeater",
    "heltec_v4_solar_repeater",
    "heltec_v4_solar_repeater_headless",
    "heltec_v4_expansionkit_repeater",
    "heltec_v4_repeater_bridge_espnow",
    "heltec_v4_repeater_bridge_espnow_low_power",
    "heltec_v4_room_server",
    "heltec_v4_terminal_chat",
    "heltec_v4_companion_radio_usb",
    "heltec_v4_companion_radio_ble",
    "heltec_v4_companion_radio_ble_low_power",
    "heltec_v4_companion_radio_wifi",
    "heltec_v4_companion_radio_wifi_low_power",
    "heltec_v4_sensor",
    "heltec_v4_sensor_low_power",
    "heltec_v4_kiss_modem",
]

build_sh = read("build.sh")
start = build_sh.index("readonly SUPPORTED_TARGETS=(")
end = build_sh.index(")\n\nif ! command", start) + 1
target_block = "readonly SUPPORTED_TARGETS=(\n" + "".join(f"  {t}\n" for t in targets) + ")"
write("build.sh", build_sh[:start] + target_block + build_sh[end:])

for rel in (
    ".github/workflows/heltec-v4-ci.yml",
    ".github/workflows/heltec-v4-validation-release.yml",
):
    text = read(rel)
    target_yaml = "".join(f"          - {t}\n" for t in targets)
    text = re.sub(
        r"(        environment:\n)(?:          - heltec_v4_[a-z0-9_]+\n?)+",
        lambda m: m.group(1) + target_yaml,
        text,
    )
    if rel.endswith("heltec-v4-ci.yml"):
        expected = "".join(f"            {t}\n" for t in targets)
        text = re.sub(
            r"(          expected_targets=\(\n)(?:            heltec_v4_[a-z0-9_]+\n)+(\s+\))",
            lambda m: m.group(1) + expected + m.group(2),
            text,
            count=1,
        )
    write(rel, text)

stable_workflow = r'''name: Heltec V4 Energy Release

on:
  push:
    branches:
      - "release/heltec-v4-energy-v*"

permissions:
  contents: write

concurrency:
  group: heltec-v4-energy-release-${{ github.ref }}
  cancel-in-progress: false

jobs:
  prepare:
    runs-on: ubuntu-latest
    outputs:
      tag: ${{ steps.meta.outputs.tag }}
    steps:
      - id: meta
        shell: bash
        run: |
          set -euo pipefail
          tag="${GITHUB_REF_NAME#release/}"
          echo "tag=${tag}" >> "${GITHUB_OUTPUT}"

  build:
    needs: prepare
    name: Package ${{ matrix.environment }}
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        environment:
''' + "".join(f"          - {t}\n" for t in targets) + r'''
    steps:
      - uses: actions/checkout@v6

      - uses: ./.github/actions/setup-build-environment

      - name: Build OTA and merged images
        shell: bash
        env:
          TARGET: ${{ matrix.environment }}
          RELEASE_TAG: ${{ needs.prepare.outputs.tag }}
        run: |
          set -euo pipefail
          short_sha="$(git rev-parse --short=8 HEAD)"
          build_date="$(date -u '+%d-%b-%Y')"
          firmware_version="${RELEASE_TAG}-${short_sha}"
          export PLATFORMIO_BUILD_FLAGS="${PLATFORMIO_BUILD_FLAGS:-} -DFIRMWARE_BUILD_DATE='\"${build_date}\"' -DFIRMWARE_VERSION='\"${firmware_version}\"'"

          pio run -e "${TARGET}"
          pio run -e "${TARGET}" -t mergebin

          mkdir -p out
          base="${TARGET}-${RELEASE_TAG}-${short_sha}"
          cp ".pio/build/${TARGET}/firmware.bin" "out/${base}-ota.bin"
          cp ".pio/build/${TARGET}/firmware-merged.bin" "out/${base}-merged.bin"
          (
            cd out
            sha256sum "${base}-ota.bin" "${base}-merged.bin" > "${base}.sha256"
          )
          cat > "out/${base}.txt" <<EOF
          Target: ${TARGET}
          Release: ${RELEASE_TAG}
          Commit: $(git rev-parse HEAD)
          Build date (UTC): ${build_date}

          ${base}-ota.bin
            Application image for compatible OTA/update paths. Do not flash at 0x0.

          ${base}-merged.bin
            Complete ESP32-S3 image. Flash at address 0x0.

          RF policy:
            The historical tx setting remains SX1262 input power.
            New installs default to 11 dBm input. FEM-aware limiting keeps
            estimated antenna output at or below 22 dBm.
          EOF

      - uses: actions/upload-artifact@v7
        with:
          name: ${{ matrix.environment }}
          path: out/*
          if-no-files-found: error

  release:
    needs: [prepare, build]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v8
        with:
          merge-multiple: true
          path: out

      - name: Create checksummed release index
        shell: bash
        env:
          RELEASE_TAG: ${{ needs.prepare.outputs.tag }}
        run: |
          set -euo pipefail
          {
            echo "# ${RELEASE_TAG}"
            echo
            echo "Commit: ${GITHUB_SHA}"
            echo
            echo "## Files"
            find out -maxdepth 1 -type f -printf '%f\n' | sort | sed 's/^/- /'
          } > out/RELEASE_INDEX.md

      - uses: softprops/action-gh-release@v3
        with:
          tag_name: ${{ needs.prepare.outputs.tag }}
          target_commitish: ${{ github.sha }}
          name: "Heltec V4 Energy Optimized ${{ needs.prepare.outputs.tag }}"
          prerelease: true
          draft: false
          fail_on_unmatched_files: true
          body: |
            Energy-optimized release candidate for the Heltec WiFi LoRa 32 V4 OLED-only fork.

            Highlights:
            - Role-specific sensor dependencies and I2C discovery.
            - Battery ADC caching while safety checks remain fresh.
            - Cooperative ESP32 idle, DFS hooks and selected automatic light sleep.
            - BLE fast/slow advertising and adaptive connection intervals.
            - Wi-Fi modem sleep and reconnect backoff.
            - Optional ESP-NOW receive-window and deep-sleep Sensor variants.
            - Solar power saving enabled by default, plus an optional headless build.
            - RX profiles: range, balanced and battery.
            - New-install SX1262 default increased from 10 to 11 dBm; detected-FEM
              limiting retains the 22 dBm estimated antenna-output ceiling.
            - A PA-settle guard protects the first LoRa preamble symbols.

            This is a prerelease because CI can validate code and packaging but cannot
            replace current, RF-output, BLE reconnection, solar and sensor measurements
            on physical V4.2/V4.3 hardware.

            Use `-ota.bin` only with a compatible update path. Use `-merged.bin` for a
            fresh ESP32-S3 flash at address `0x0`.
          files: out/*
'''
write(".github/workflows/heltec-v4-energy-release.yml", stable_workflow)


# ---------------------------------------------------------------------------
# Documentation.
# ---------------------------------------------------------------------------

audit_doc = r'''# Heltec V4 energy optimization implementation

This implementation preserves the historical MeshCore `tx` meaning (SX1262
input dBm) and retains the detected-FEM estimated antenna-output ceiling of
22 dBm.

## Common changes

- The default SX1262 input is 11 dBm on new installs. That reaches the existing
  22 dBm estimate on both GC1109 and KCT8103L paths; persisted settings are not
  overwritten.
- A 20 microsecond FEM settle guard is applied before LoRa transmission begins.
- Battery measurements are cached for ten seconds for UI and telemetry. Solar
  cutoff checks and OTA qualification request a fresh measurement.
- Every application yields to the ESP32 FreeRTOS idle task. The duplicate board
  loop inside the RadioLib wrapper was removed.
- DFS is enabled where the framework supports it. Automatic light sleep is
  enabled only on targets whose transport is designed for it.
- Noise-floor calibration is fast for the first minute, then changes from a
  two-second to a thirty-second cadence.
- External RTC and environmental-sensor probing are compiled only into targets
  that request them.
- OLED VEXT ownership is reference-counted by the board/accessory and display
  independently.
- GPS UART is stopped when GPS is disabled and restarted when enabled.

## Connectivity

BLE advertises quickly for one minute after boot or disconnect, then changes to
a slower interval. Connected links use interactive parameters during transfers
and request lower-event-rate parameters after ten seconds of inactivity.

Wi-Fi enables modem sleep and uses exponential reconnect backoff. The
`wifi_low_power` variant permits automatic light sleep and a longer backoff cap.

The standard ESP-NOW bridge remains continuously reachable. The
`espnow_low_power` variant requests a 20 ms receive window every 100 ms and can
therefore miss uncoordinated packets; it is for coordinated battery deployments.

## RX profiles

The default remains `range`, preserving the existing boosted SX1262 receiver and
the KCT8103L external LNA. Runtime profiles are:

- `range`: SX1262 boosted RX plus external LNA where controllable.
- `balanced`: normal SX1262 RX plus external LNA.
- `battery`: normal SX1262 RX and external-LNA bypass where controllable.

Commands:

```text
get radio.rxprofile
set radio.rxprofile range
set radio.rxprofile balanced
set radio.rxprofile battery
```

RX profiles never reduce transmit power.

## Specialized targets

- `heltec_v4_sensor_low_power` wakes for 12 seconds every 15 minutes, powers the
  sensor rail during the wake window, samples, advertises, services queued work,
  then deep sleeps. It is not continuously reachable.
- `heltec_v4_solar_repeater_headless` removes application USB CDC and PSRAM
  initialization. Use the normal Solar Repeater when a runtime USB console is
  required.
- Low-power BLE, Wi-Fi and ESP-NOW variants retain their corresponding standard
  variants for direct A/B validation.

## Required physical validation

CI verifies compilation, policy tests and image packaging. Release qualification
still requires current and RF measurements for V4.2 and V4.3, BLE reconnect
tests, Wi-Fi/ESP-NOW traffic tests, sensor wake/sleep tests and the full
3.50/3.65 V solar recovery cycle.
'''
write("docs/HELTEC_V4_ENERGY_OPTIMIZATION.md", audit_doc)

readme = read("README.md")
readme = readme.replace(
    "| `heltec_v4_solar_repeater` | Unattended solar repeater with protective battery recovery |\n",
    "| `heltec_v4_solar_repeater` | Unattended solar repeater with protective battery recovery |\n"
    "| `heltec_v4_solar_repeater_headless` | Solar repeater without application USB CDC or PSRAM initialization |\n",
    1,
)
readme = readme.replace(
    "| `heltec_v4_repeater_bridge_espnow` | ESP-NOW bridge repeater |\n",
    "| `heltec_v4_repeater_bridge_espnow` | ESP-NOW bridge repeater |\n"
    "| `heltec_v4_repeater_bridge_espnow_low_power` | Coordinated ESP-NOW receive-window variant |\n",
    1,
)
readme = readme.replace(
    "| `heltec_v4_companion_radio_ble` | Bluetooth companion node |\n",
    "| `heltec_v4_companion_radio_ble` | Bluetooth companion node |\n"
    "| `heltec_v4_companion_radio_ble_low_power` | BLE Companion with automatic light-sleep hook |\n",
    1,
)
readme = readme.replace(
    "| `heltec_v4_companion_radio_wifi` | Wi-Fi companion node |\n",
    "| `heltec_v4_companion_radio_wifi` | Wi-Fi companion node |\n"
    "| `heltec_v4_companion_radio_wifi_low_power` | Wi-Fi modem-sleep and extended-backoff variant |\n",
    1,
)
readme = readme.replace(
    "| `heltec_v4_sensor` | Sensor node |\n",
    "| `heltec_v4_sensor` | Always-on sensor node |\n"
    "| `heltec_v4_sensor_low_power` | 15-minute deep-sleep sensor profile |\n",
    1,
)
readme = readme.replace(
    "- Calibrated 12-bit battery sampling using 15 averaged ADC readings.\n",
    "- Calibrated 12-bit battery sampling using 15 averaged ADC readings, with a ten-second telemetry/UI cache and uncached safety checks.\n"
    "- Cooperative ESP32 idle on every role, optional DFS/automatic light sleep, and role-specific sensor/RTC discovery.\n"
    "- BLE fast/slow advertising, adaptive connection intervals, Wi-Fi modem sleep/backoff, and optional ESP-NOW receive windows.\n",
    1,
)
readme = readme.replace(
    "The existing MeshCore `tx` setting keeps its historical meaning: requested **SX1262 input power**, not antenna output. The board may lower the applied value according to the detected FEM. For example, the default request of 10 dBm remains 10 dBm on GC1109 but is limited to 9 dBm on KCT8103L.\n\n"
    "The current Arduino-ESP32 framework used by this repository does not enable ESP-IDF dynamic frequency scaling. Heltec V4 therefore remains at the existing fixed 80 MHz CPU clock; automatic light sleep is not introduced.\n",
    "The existing MeshCore `tx` setting keeps its historical meaning: requested **SX1262 input power**, not antenna output. New installations default to 11 dBm. FEM-aware limiting applies 11 dBm on GC1109 and 9 dBm on KCT8103L, producing the existing 22 dBm estimated antenna-output ceiling on either revision. Persisted `tx` values are preserved. A short FEM-settle guard is applied before the first preamble symbol.\n\n"
    "The CPU remains capped at 80 MHz. Targets that support it request 40–80 MHz dynamic frequency scaling; selected low-power targets also request automatic light sleep. Every role yields to FreeRTOS idle even when framework-level PM is unavailable.\n",
    1,
)
readme = readme.replace(
    "get radio.fem.rxgain\n",
    "get radio.fem.rxgain\nget radio.rxprofile\nset radio.rxprofile range\nset radio.rxprofile balanced\nset radio.rxprofile battery\n",
    1,
)
readme = readme.replace(
    "This profile is intentionally limited to the repeater application. Use a standard Companion target when continuous BLE, USB or Wi-Fi availability is required.\n",
    "This profile is intentionally limited to the repeater application. Power saving is enabled by default after the startup window. Use a standard Companion target when continuous BLE, USB or Wi-Fi availability is required. The optional headless target removes application USB CDC and PSRAM initialization for unattended deployments.\n",
    1,
)
readme += "\n## Energy optimization details\n\nSee [`docs/HELTEC_V4_ENERGY_OPTIMIZATION.md`](docs/HELTEC_V4_ENERGY_OPTIMIZATION.md) for the implementation matrix, trade-offs and physical validation requirements.\n"
write("README.md", readme)

about = read("ABOUT.md")
if "Energy optimization generation" not in about:
    about += r'''

## Energy optimization generation

The current generation adds role-specific peripheral builds, cached battery
telemetry with fresh safety readings, cooperative ESP32 idle, optional DFS and
automatic light sleep, BLE/Wi-Fi/ESP-NOW power policies, runtime RX profiles and
a deep-sleep Sensor target. New installations request 11 dBm SX1262 input while
the detected-FEM policy retains the 22 dBm estimated antenna-output ceiling.
'''
write("ABOUT.md", about)

release_md = read("RELEASE.md")
if "## Energy release" not in release_md:
    release_md += r'''

## Energy release

Create a branch named `release/heltec-v4-energy-vX.Y.Z-rcN` from the audited
commit. The energy release workflow builds every retained environment and
publishes checksummed `-ota.bin` and `-merged.bin` assets. The OTA image must not
be flashed at address `0x0`; the merged image is the complete image for `0x0`.
'''
write("RELEASE.md", release_md)

print("Heltec V4 energy optimization transformation completed.")
