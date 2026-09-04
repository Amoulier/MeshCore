from pathlib import Path

path = Path("variants/heltec_v4/HeltecV4Board.cpp")
text = path.read_text(encoding="utf-8")

old_helpers = '''void releasePinHold(int pin)
{
  if (pin >= 0) {
    gpio_hold_dis(static_cast<gpio_num_t>(pin));
  }
}

void configureAndHoldPin(int pin, uint8_t level)
{
  if (pin < 0) {
    return;
  }

  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  gpio_hold_dis(gpio);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_en(gpio);
}
'''
new_helpers = '''void releasePinHoldAtLevel(int pin, uint8_t level)
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
'''
if text.count(old_helpers) != 1:
    raise RuntimeError("helper block did not match exactly once")
text = text.replace(old_helpers, new_helpers)

old_release = '''void HeltecV4Board::releaseCriticalBatteryHolds()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  gpio_deep_sleep_hold_dis();
  releasePinHold(PIN_ADC_CTRL);
  releasePinHold(PIN_VBAT_READ);
  releasePinHold(PIN_VEXT_EN);
#ifdef PIN_OLED_RESET
  releasePinHold(PIN_OLED_RESET);
#endif
#ifdef PIN_GPS_EN
  releasePinHold(PIN_GPS_EN);
#endif
#ifdef PIN_GPS_RESET
  releasePinHold(PIN_GPS_RESET);
#endif
#ifdef P_LORA_TX_LED
  releasePinHold(P_LORA_TX_LED);
#endif
  releasePinHold(P_LORA_NSS);
  releasePinHold(P_LORA_PA_POWER);
  releasePinHold(P_LORA_GC1109_PA_EN);
  releasePinHold(P_LORA_GC1109_PA_TX_EN);
  releasePinHold(P_LORA_KCT8103L_PA_CTX);
#endif
}
'''
new_release = '''void HeltecV4Board::releaseCriticalBatteryHolds()
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
  releasePinHoldAtLevel(P_LORA_PA_POWER, LOW);
  releasePinHoldAtLevel(P_LORA_GC1109_PA_EN, LOW);
  releasePinHoldAtLevel(P_LORA_GC1109_PA_TX_EN, LOW);
  releasePinHoldAtLevel(P_LORA_KCT8103L_PA_CTX, HIGH);
}
'''
if text.count(old_release) != 1:
    raise RuntimeError("hold release block did not match exactly once")
text = text.replace(old_release, new_release)

old_begin = '''void HeltecV4Board::begin()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  const bool recovery_was_latched = battery_critical_latched;
  releaseCriticalBatteryHolds();
  persistent_writes_allowed = true;

  const uint16_t boot_millivolts = readBatteryMilliVoltsRaw();
  if (heltec_v4::shouldUseCriticalBatteryRecovery(
          boot_millivolts, recovery_was_latched,
          HELTEC_V4_BATTERY_BOOT_GUARD_MIN_MILLIVOLTS,
          HELTEC_V4_BATTERY_CRITICAL_MILLIVOLTS,
          HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS)) {
    enterCriticalBatterySleep(false);
  }
  battery_critical_latched = false;
#endif

  ESP32Board::begin();
'''
new_begin = '''void HeltecV4Board::begin()
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

  const uint16_t boot_millivolts = readBatteryMilliVoltsRaw();
  if (heltec_v4::shouldUseCriticalBatteryRecovery(
          boot_millivolts, recovery_was_latched,
          HELTEC_V4_BATTERY_BOOT_GUARD_MIN_MILLIVOLTS,
          HELTEC_V4_BATTERY_CRITICAL_MILLIVOLTS,
          HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS)) {
    enterCriticalBatterySleep(false);
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
'''
if text.count(old_begin) != 1:
    raise RuntimeError("begin block did not match exactly once")
text = text.replace(old_begin, new_begin)

path.write_text(text, encoding="utf-8")
print("Heltec V4 recovery hold sequencing patched")
