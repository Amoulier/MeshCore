from pathlib import Path

path = Path("variants/heltec_v4/HeltecV4Board.cpp")
text = path.read_text(encoding="utf-8")

old_poweroff = '''void HeltecV4Board::powerOff()
{
  heltecV4CriticalPreSleep();
  loRaFEMControl.setSleepModeEnable();
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
  configureAndHoldPin(P_LORA_GC1109_PA_TX_EN, LOW);
  configureAndHoldPin(P_LORA_GC1109_PA_EN, LOW);
  configureAndHoldPin(P_LORA_PA_POWER, LOW);
  gpio_deep_sleep_hold_en();
  ESP32Board::powerOff();
}
'''
new_poweroff = '''void HeltecV4Board::powerOff()
{
  persistent_writes_allowed = false;
  heltecV4CriticalPreSleep();
  loRaFEMControl.setSleepModeEnable();
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
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
}
'''
if text.count(old_poweroff) != 1:
    raise RuntimeError("powerOff block did not match exactly once")
text = text.replace(old_poweroff, new_poweroff)

old_attach = '''void HeltecV4Board::attachDynamicPrefs(KeyValueStore *prefs)
{
  _prefs = prefs;
}
'''
new_attach = '''void HeltecV4Board::attachDynamicPrefs(KeyValueStore *prefs)
{
  _prefs = prefs;
  if (_prefs && loRaFEMControl.isLnaCanControl()) {
    _prefs->setByKey("fem_rxgain", isLoRaFemLnaEnabled() ? "1" : "0");
  }
}
'''
if text.count(old_attach) != 1:
    raise RuntimeError("attachDynamicPrefs block did not match exactly once")
text = text.replace(old_attach, new_attach)

path.write_text(text, encoding="utf-8")
print("Heltec V4 shutdown and LNA preference sync patched")
