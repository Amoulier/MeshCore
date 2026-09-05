#include "HeltecV4Board.h"

#include <Preferences.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_idf_version.h>
#include <esp_sleep.h>
#if defined(CONFIG_PM_ENABLE) && CONFIG_PM_ENABLE
#include <esp_pm.h>
#endif

#include <helpers/PersistentWriteGuard.h>

#ifndef HELTEC_V4_BATTERY_CRITICAL_MILLIVOLTS
#define HELTEC_V4_BATTERY_CRITICAL_MILLIVOLTS 3500
#endif
#ifndef HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS
#define HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS 3650
#endif
#ifndef HELTEC_V4_BATTERY_CRITICAL_READINGS
#define HELTEC_V4_BATTERY_CRITICAL_READINGS 3
#endif
#ifndef HELTEC_V4_BATTERY_CHECK_INTERVAL_MSEC
#define HELTEC_V4_BATTERY_CHECK_INTERVAL_MSEC 8000UL
#endif
#ifndef HELTEC_V4_BATTERY_RECOVERY_SLEEP_MSEC
#define HELTEC_V4_BATTERY_RECOVERY_SLEEP_MSEC 60000UL
#endif
#ifndef HELTEC_V4_BATTERY_BOOT_GUARD_MIN_MILLIVOLTS
#define HELTEC_V4_BATTERY_BOOT_GUARD_MIN_MILLIVOLTS 2500
#endif
#ifndef HELTEC_V4_MAX_OUTPUT_POWER_DBM
#define HELTEC_V4_MAX_OUTPUT_POWER_DBM 22
#endif
#ifndef HELTEC_V4_BATTERY_CACHE_MSEC
#define HELTEC_V4_BATTERY_CACHE_MSEC 10000UL
#endif
#ifndef HELTEC_V4_TX_PA_SETTLE_US
#define HELTEC_V4_TX_PA_SETTLE_US 20U
#endif
#ifndef HELTEC_V4_CPU_MAX_MHZ
#define HELTEC_V4_CPU_MAX_MHZ 80
#endif
#ifndef HELTEC_V4_CPU_MIN_MHZ
#define HELTEC_V4_CPU_MIN_MHZ 40
#endif

__attribute__((noinline)) bool heltecV4SetDisplayEnabled(bool enabled) __attribute__((weak));
__attribute__((noinline)) bool heltecV4SetDisplayEnabled(bool enabled)
{
  (void)enabled;
  return false;
}

__attribute__((noinline)) int8_t heltecV4GetDisplayDisabled() __attribute__((weak));
__attribute__((noinline)) int8_t heltecV4GetDisplayDisabled()
{
  return -1;
}

__attribute__((noinline)) void heltecV4CriticalPreSleep() __attribute__((weak));
__attribute__((noinline)) void heltecV4CriticalPreSleep() {}

__attribute__((noinline)) bool heltecV4SetInternalRxBoosted(bool enabled) __attribute__((weak));
__attribute__((noinline)) bool heltecV4SetInternalRxBoosted(bool enabled)
{
  (void)enabled;
  return false;
}

__attribute__((noinline)) int8_t heltecV4GetInternalRxBoosted() __attribute__((weak));
__attribute__((noinline)) int8_t heltecV4GetInternalRxBoosted()
{
  return -1;
}

namespace {

RTC_DATA_ATTR bool battery_critical_latched = false;
volatile bool persistent_writes_allowed = true;
bool fem_lna_nvs_explicit = false;

constexpr const char *BOARD_PREFS_NAMESPACE = "meshcore_v4";
constexpr const char *FEM_LNA_EXPLICIT_KEY = "fem_lna_set";
constexpr const char *FEM_LNA_ENABLED_KEY = "fem_lna";

void releasePinHoldAtLevel(int pin, uint8_t level)
{
  if (pin < 0) {
    return;
  }

  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  // Program the post-hold state before releasing the pad to avoid a transient
  // back to the reset default during a recovery wake.
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_dis(gpio);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
}

void configureAndHoldPin(int pin, uint8_t level)
{
  if (pin < 0) {
    return;
  }

  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_dis(gpio);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_en(gpio);
}

heltec_v4::PowerProfile configuredPowerProfile()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  return heltec_v4::PowerProfile::SolarRepeater;
#else
  return heltec_v4::PowerProfile::Standard;
#endif
}

bool persistFemLnaPreference(bool enabled)
{
  if (!meshcorePersistentWritesAllowed()) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(BOARD_PREFS_NAMESPACE, false)) {
    return false;
  }
  // The explicit marker is the commit record: write the value first so
  // an interrupted migration is retried rather than accepting a default.
  const bool value_written = preferences.putBool(FEM_LNA_ENABLED_KEY, enabled) == sizeof(bool);
  const bool marker_written = value_written &&
                              preferences.putBool(FEM_LNA_EXPLICIT_KEY, true) == sizeof(bool);
  preferences.end();
  return marker_written;
}

} // namespace

bool meshcorePersistentWritesAllowed()
{
  return persistent_writes_allowed;
}

void HeltecV4Board::releaseCriticalBatteryHolds()
{
  gpio_deep_sleep_hold_dis();
  releasePinHoldAtLevel(PIN_ADC_CTRL, LOW);
  releasePinHoldAtLevel(PIN_VEXT_EN, HIGH);
#ifdef PIN_OLED_RESET
  releasePinHoldAtLevel(PIN_OLED_RESET, LOW);
#endif
#ifdef PIN_GPS_EN
  releasePinHoldAtLevel(PIN_GPS_EN, !PIN_GPS_EN_ACTIVE);
#endif
#ifdef PIN_GPS_RESET
  releasePinHoldAtLevel(PIN_GPS_RESET, PIN_GPS_RESET_ACTIVE);
#endif
#ifdef P_LORA_TX_LED
  releasePinHoldAtLevel(P_LORA_TX_LED, LOW);
#endif
  releasePinHoldAtLevel(P_LORA_NSS, HIGH);
  releasePinHoldAtLevel(P_LORA_RESET, HIGH);
  releasePinHoldAtLevel(P_LORA_PA_POWER, LOW);
  releasePinHoldAtLevel(P_LORA_GC1109_PA_EN, LOW);
  releasePinHoldAtLevel(P_LORA_GC1109_PA_TX_EN, LOW);
  releasePinHoldAtLevel(P_LORA_KCT8103L_PA_CTX, HIGH);
}

uint16_t HeltecV4Board::readBatteryMilliVoltsRaw()
{
  pinMode(PIN_ADC_CTRL, OUTPUT);
  digitalWrite(PIN_ADC_CTRL, HIGH);
  delay(10);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_VBAT_READ, ADC_2_5db);

  uint32_t pin_millivolts = 0;
  constexpr uint8_t samples = 15;
  for (uint8_t sample = 0; sample < samples; sample++) {
    pin_millivolts += analogReadMilliVolts(PIN_VBAT_READ);
  }

  digitalWrite(PIN_ADC_CTRL, LOW);
  const float battery_millivolts = (pin_millivolts / static_cast<float>(samples)) * adc_mult;
  return static_cast<uint16_t>(battery_millivolts + 0.5f);
}

void HeltecV4Board::updateReportedBatteryPercent(uint16_t battery_millivolts)
{
  const uint8_t target_percent = heltec_v4::batteryPercent(battery_millivolts, configuredPowerProfile());
  const uint32_t now = millis();

  if (reported_battery_percent < 0) {
    reported_battery_percent = target_percent;
    last_battery_percent_change = now;
    return;
  }

  const uint32_t elapsed = now - last_battery_percent_change;
  const uint8_t next_percent = heltec_v4::slewBatteryPercent(
      static_cast<uint8_t>(reported_battery_percent), target_percent, elapsed,
      BATTERY_PERCENT_SLEW_INTERVAL_MSEC);
  if (next_percent != reported_battery_percent) {
    reported_battery_percent = next_percent;
    last_battery_percent_change = now;
  }
}

void HeltecV4Board::configureCpuPowerManagement()
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

void HeltecV4Board::enterCriticalBatterySleep(bool runtime_shutdown, bool force_radio_reset)
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  persistent_writes_allowed = false;
  battery_critical_latched = true;

  if (runtime_shutdown) {
    heltecV4CriticalPreSleep();
    loRaFEMControl.setSleepModeEnable();
  }

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
  if (force_radio_reset) {
    const gpio_num_t radio_reset = static_cast<gpio_num_t>(P_LORA_RESET);
    gpio_pullup_dis(radio_reset);
    gpio_pulldown_dis(radio_reset);
    configureAndHoldPin(P_LORA_RESET, LOW);
  }
  configureAndHoldPin(P_LORA_GC1109_PA_TX_EN, LOW);
  configureAndHoldPin(P_LORA_KCT8103L_PA_CTX, HIGH);
  configureAndHoldPin(P_LORA_GC1109_PA_EN, LOW);
  configureAndHoldPin(P_LORA_PA_POWER, LOW);
  gpio_deep_sleep_hold_en();

  Serial.flush();
  delay(20);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(HELTEC_V4_BATTERY_RECOVERY_SLEEP_MSEC) * 1000ULL);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
  esp_deep_sleep_start();
#else
  (void)runtime_shutdown;
  (void)force_radio_reset;
#endif
}

void HeltecV4Board::begin()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  const bool recovery_was_latched = battery_critical_latched;
  persistent_writes_allowed = true;

  if (recovery_was_latched) {
    // Leave OLED, GPS and FEM held in their low-power state. Only the ADC gate
    // is released for the recovery measurement.
    gpio_deep_sleep_hold_dis();
    releasePinHoldAtLevel(PIN_ADC_CTRL, LOW);
  } else {
    releaseCriticalBatteryHolds();
  }

  const bool timer_wake = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
  const bool radio_state_known_safe = heltec_v4::isBatteryRecoveryRadioStateKnownSafe(
      recovery_was_latched, timer_wake);
  const uint16_t boot_millivolts = getBattMilliVoltsFresh();
  if (heltec_v4::shouldUseCriticalBatteryRecovery(
          boot_millivolts, recovery_was_latched,
          HELTEC_V4_BATTERY_BOOT_GUARD_MIN_MILLIVOLTS,
          HELTEC_V4_BATTERY_CRITICAL_MILLIVOLTS,
          HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS)) {
    enterCriticalBatterySleep(false, !radio_state_known_safe);
  }

  if (recovery_was_latched) {
    releaseCriticalBatteryHolds();
  }
  battery_critical_latched = false;
#else
  // Clear any retained shutdown pads before normal initialization.
  releaseCriticalBatteryHolds();
#endif

  ESP32Board::begin();

  pinMode(PIN_ADC_CTRL, OUTPUT);
  digitalWrite(PIN_ADC_CTRL, LOW);

#ifdef P_LORA_TX_LED
  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, LOW);
#endif

  loRaFEMControl.init();
  bool enable_lna = true;
  fem_lna_nvs_explicit = false;
  Preferences preferences;
  if (preferences.begin(BOARD_PREFS_NAMESPACE, true)) {
    fem_lna_nvs_explicit = preferences.getBool(FEM_LNA_EXPLICIT_KEY, false);
    if (fem_lna_nvs_explicit) {
      enable_lna = preferences.getBool(FEM_LNA_ENABLED_KEY, true);
    }
    preferences.end();
  }
  setLoRaFemLnaEnabled(enable_lna);

  periph_power.begin();
#if defined(HELTEC_V4_KEEP_ACCESSORY_RAIL_ON) && HELTEC_V4_KEEP_ACCESSORY_RAIL_ON
  // The board, not the OLED driver, owns the permanent accessory claim.
  periph_power.claim();
#endif
  configureCpuPowerManagement();

  const esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_DEEPSLEEP) {
    const uint64_t wakeup_source = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_source & (1ULL << P_LORA_DIO_1)) {
      startup_reason = BD_STARTUP_RX_PACKET;
    }

    gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_NSS));
    rtc_gpio_deinit(static_cast<gpio_num_t>(P_LORA_DIO_1));
  }
}

void HeltecV4Board::loop()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  if (inhibit_sleep) {
    return;
  }

  const uint32_t now = millis();
  if (now - last_critical_battery_check < HELTEC_V4_BATTERY_CHECK_INTERVAL_MSEC) {
    return;
  }
  last_critical_battery_check = now;

  const uint16_t battery_millivolts = getBattMilliVoltsFresh();
  critical_low_readings = heltec_v4::updateLowReadingCounter(
      critical_low_readings, battery_millivolts,
      HELTEC_V4_BATTERY_BOOT_GUARD_MIN_MILLIVOLTS,
      HELTEC_V4_BATTERY_CRITICAL_MILLIVOLTS,
      HELTEC_V4_BATTERY_CRITICAL_READINGS);
  if (critical_low_readings >= HELTEC_V4_BATTERY_CRITICAL_READINGS) {
    enterCriticalBatterySleep(true, false);
  }
#endif
}

void HeltecV4Board::onBeforeTransmit()
{
#if defined(P_LORA_TX_LED) && !(defined(HELTEC_V4_DISABLE_TX_LED) && HELTEC_V4_DISABLE_TX_LED)
  digitalWrite(P_LORA_TX_LED, HIGH);
#endif
  loRaFEMControl.setTxModeEnable();
#if HELTEC_V4_TX_PA_SETTLE_US > 0
  // Let the detected external FEM reach its TX state before the first preamble symbol.
  delayMicroseconds(HELTEC_V4_TX_PA_SETTLE_US);
#endif
}

void HeltecV4Board::onAfterTransmit()
{
#if defined(P_LORA_TX_LED) && !(defined(HELTEC_V4_DISABLE_TX_LED) && HELTEC_V4_DISABLE_TX_LED)
  digitalWrite(P_LORA_TX_LED, LOW);
#endif
  loRaFEMControl.setRxModeEnable();
}

bool HeltecV4Board::startOTAUpdate(const char *id, char reply[])
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  const uint16_t battery_millivolts = getBattMilliVoltsFresh();
  if (battery_millivolts < HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS) {
    sprintf(reply, "Error: battery %umV; OTA requires at least %umV",
            static_cast<unsigned>(battery_millivolts),
            static_cast<unsigned>(HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS));
    return false;
  }
#endif
  return ESP32Board::startOTAUpdate(id, reply);
}

void HeltecV4Board::enterDeepSleep(uint32_t secs)
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

int8_t HeltecV4Board::getBattPercent()
{
  if (reported_battery_percent < 0) {
    getBattMilliVolts();
  }
  return static_cast<int8_t>(reported_battery_percent);
}

void HeltecV4Board::idle(uint32_t delay_millis)
{
  // Arduino delay yields to the FreeRTOS idle task, allowing DFS and
  // automatic light sleep where the selected target enables them.
  delay(delay_millis == 0 ? 1 : delay_millis);
}

int8_t HeltecV4Board::mapRadioTxPower(int8_t requested_radio_dbm)
{
  if (requested_radio_dbm < -9) {
    requested_radio_dbm = -9;
  } else if (requested_radio_dbm > 22) {
    requested_radio_dbm = 22;
  }

  last_requested_radio_dbm = requested_radio_dbm;
  if (loRaFEMControl.getFEMType() == GC1109_PA) {
    last_radio_input_dbm = heltec_v4::clampGc1109RadioInput(
        requested_radio_dbm, HELTEC_V4_MAX_OUTPUT_POWER_DBM);
  } else if (loRaFEMControl.getFEMType() == KCT8103L_PA) {
    last_radio_input_dbm = heltec_v4::clampKct8103lRadioInput(
        requested_radio_dbm, HELTEC_V4_MAX_OUTPUT_POWER_DBM);
  } else {
    last_radio_input_dbm = requested_radio_dbm;
  }
  return last_radio_input_dbm;
}

const char *HeltecV4Board::getManufacturerName() const
{
#ifdef HELTEC_LORA_V4_TFT
  return loRaFEMControl.getFEMType() == KCT8103L_PA ? "Heltec V4.3 TFT" : "Heltec V4 TFT";
#else
  return loRaFEMControl.getFEMType() == KCT8103L_PA ? "Heltec V4.3 OLED" : "Heltec V4 OLED";
#endif
}

bool HeltecV4Board::setLoRaFemLnaEnabled(bool enable)
{
  if (!loRaFEMControl.isLnaCanControl()) {
    return false;
  }

  loRaFEMControl.setLNAEnable(enable);
  loRaFEMControl.setRxModeEnable();
  return true;
}

bool HeltecV4Board::isLoRaFemLnaEnabled() const
{
  return loRaFEMControl.isLNAEnabled();
}

void HeltecV4Board::attachDynamicPrefs(KeyValueStore *prefs)
{
  _prefs = prefs;
  if (!_prefs || !loRaFEMControl.isLnaCanControl()) {
    return;
  }

  if (fem_lna_nvs_explicit) {
    // Once migrated, NVS is authoritative. Only touch the legacy JSON
    // field when it actually differs, avoiding an unnecessary flash save.
    const char *desired_fem_rxgain = isLoRaFemLnaEnabled() ? "1" : "0";
    char stored_fem_rxgain[8] = {};
    if (!_prefs->getByKey("fem_rxgain", stored_fem_rxgain, sizeof(stored_fem_rxgain)) ||
        strcmp(stored_fem_rxgain, desired_fem_rxgain) != 0) {
      _prefs->setByKey("fem_rxgain", desired_fem_rxgain);
    }
    return;
  }

  char legacy_fem_rxgain[8] = {};
  if (!_prefs->getByKey("fem_rxgain", legacy_fem_rxgain, sizeof(legacy_fem_rxgain))) {
    return;
  }

  // Fresh installs default this field to 1; upgraded installations can
  // carry an explicit 0. Apply it before creating the new NVS marker.
  const bool enable_lna = strcmp(legacy_fem_rxgain, "1") == 0;
  setLoRaFemLnaEnabled(enable_lna);
  fem_lna_nvs_explicit = persistFemLnaPreference(enable_lna);
}

bool HeltecV4Board::handleCommand(const char *command, uint32_t sender_timestamp, char *reply)
{
  (void)sender_timestamp;

  if (strcmp(command, "get radio.fem.rxgain") == 0) {
    if (!loRaFEMControl.isLnaCanControl()) {
      strcpy(reply, "Error: unsupported");
    } else {
      sprintf(reply, "> %s", isLoRaFemLnaEnabled() ? "on" : "off");
    }
    return true;
  }

  if (memcmp(command, "set radio.fem.rxgain ", 21) == 0) {
    if (!loRaFEMControl.isLnaCanControl()) {
      strcpy(reply, "Error: unsupported");
      return true;
    }

    const bool enable = memcmp(&command[21], "on", 2) == 0;
    const bool disable = memcmp(&command[21], "off", 3) == 0;
    if (!enable && !disable) {
      strcpy(reply, "Error: state must be on or off");
      return true;
    }

    if (!setLoRaFemLnaEnabled(enable)) {
      strcpy(reply, "Error: failed to apply LoRa FEM RX gain");
      return true;
    }

    if (_prefs) {
      _prefs->setByKey("fem_rxgain", enable ? "1" : "0");
    }
    fem_lna_nvs_explicit = persistFemLnaPreference(enable) || fem_lna_nvs_explicit;
    strcpy(reply, enable ? "OK - LoRa FEM RX gain on" : "OK - LoRa FEM RX gain off");
    return true;
  }

  if (strcmp(command, "get radio.rxprofile") == 0) {
    const int8_t internal = heltecV4GetInternalRxBoosted();
    const bool external_supported = loRaFEMControl.isLnaCanControl();
    const bool external = !external_supported || isLoRaFemLnaEnabled();
    const char *profile = "custom";
    if (internal > 0 && external) {
      profile = "range";
    } else if (internal == 0 && external) {
      profile = "balanced";
    } else if (internal == 0 && !external) {
      profile = "battery";
    }
    sprintf(reply, "> %s (sx1262:%s,fem:%s)", profile,
            internal > 0 ? "boosted" : (internal == 0 ? "normal" : "unknown"),
            external_supported ? (external ? "lna" : "bypass") : "fixed");
    return true;
  }

  if (memcmp(command, "set radio.rxprofile ", 20) == 0) {
    const char *value = &command[20];
    const bool range = strcmp(value, "range") == 0;
    const bool balanced = strcmp(value, "balanced") == 0;
    const bool battery = strcmp(value, "battery") == 0;
    if (!range && !balanced && !battery) {
      strcpy(reply, "Error: profile must be range, balanced or battery");
      return true;
    }
    const bool internal_boost = range;
    const bool external_lna = range || balanced;
    if (!heltecV4SetInternalRxBoosted(internal_boost)) {
      strcpy(reply, "Error: failed to set SX1262 RX gain");
      return true;
    }
    if (loRaFEMControl.isLnaCanControl() && !setLoRaFemLnaEnabled(external_lna)) {
      strcpy(reply, "Error: failed to set FEM RX gain");
      return true;
    }
    if (_prefs) {
      _prefs->setByKey("rxgain", internal_boost ? "1" : "0");
      _prefs->setByKey("fem_rxgain", external_lna ? "1" : "0");
    }
    fem_lna_nvs_explicit = persistFemLnaPreference(external_lna) || fem_lna_nvs_explicit;
    sprintf(reply, "OK - RX profile %s", value);
    return true;
  }

  if (strcmp(command, "get radio.power") == 0) {
    const int8_t estimated_output_dbm = loRaFEMControl.getFEMType() == GC1109_PA
        ? heltec_v4::gc1109EstimatedOutput(last_radio_input_dbm)
        : (loRaFEMControl.getFEMType() == KCT8103L_PA
             ? heltec_v4::kct8103lEstimatedOutput(last_radio_input_dbm)
             : last_radio_input_dbm);
    sprintf(reply, "> requested-radio:%ddBm,applied-radio:%ddBm,estimated-output:%ddBm",
            last_requested_radio_dbm, last_radio_input_dbm, estimated_output_dbm);
    return true;
  }

  if (strcmp(command, "get power.status") == 0) {
    const uint16_t millivolts = getBattMilliVolts();
    sprintf(reply, "> %umV,%d%%,%s%s", static_cast<unsigned>(millivolts), getBattPercent(),
            configuredPowerProfile() == heltec_v4::PowerProfile::SolarRepeater ? "solar" : "standard",
            battery_critical_latched ? ",recovery" : "");
    return true;
  }

  if (strcmp(command, "get display") == 0) {
    const int8_t disabled = heltecV4GetDisplayDisabled();
    if (disabled < 0) {
      strcpy(reply, "Error: unsupported");
    } else {
      sprintf(reply, "> %s", disabled ? "off" : "on");
    }
    return true;
  }

  if (strcmp(command, "set display off") == 0 || strcmp(command, "display off") == 0) {
    strcpy(reply, heltecV4SetDisplayEnabled(false) ? "OK - display disabled; hold PRG to restore" : "Error: unsupported");
    return true;
  }

  if (strcmp(command, "set display on") == 0 || strcmp(command, "display on") == 0) {
    strcpy(reply, heltecV4SetDisplayEnabled(true) ? "OK - display enabled" : "Error: unsupported");
    return true;
  }

  return false;
}
