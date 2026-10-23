#include "LoRaFEMControl.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

const char *LoRaFEMControl::getFEMName() const
{
  if (fem_type == GC1109_PA) {
    return "GC1109";
  }
  if (fem_type == KCT8103L_PA) {
    return "KCT8103L";
  }
  return "none";
}

void LoRaFEMControl::init()
{
  lna_can_control = false;

#if !HELTEC_V4_HIGH_POWER
  // The official low-power V4 uses the SX1262 path directly. Keep all external
  // FEM controls inactive so a 433-510 MHz / 22 dBm board is never driven as
  // though it contained the 28 dBm GC1109 or KCT8103L front end.
  fem_type = OTHER_FEM_TYPES;
  pinMode(P_LORA_PA_POWER, OUTPUT);
  digitalWrite(P_LORA_PA_POWER, LOW);
  pinMode(P_LORA_GC1109_PA_EN, OUTPUT);
  digitalWrite(P_LORA_GC1109_PA_EN, LOW);
  pinMode(P_LORA_GC1109_PA_TX_EN, OUTPUT);
  digitalWrite(P_LORA_GC1109_PA_TX_EN, LOW);
  pinMode(P_LORA_KCT8103L_PA_CTX, OUTPUT);
  digitalWrite(P_LORA_KCT8103L_PA_CTX, HIGH);
  return;
#else
  pinMode(P_LORA_PA_POWER, OUTPUT);
  digitalWrite(P_LORA_PA_POWER, HIGH);
  gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_PA_POWER));
  pinMode(P_LORA_PA_POWER, OUTPUT);
  digitalWrite(P_LORA_PA_POWER, HIGH);
  delay(5);

  gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_KCT8103L_PA_CSD));
  pinMode(P_LORA_KCT8103L_PA_CSD, INPUT);
  delay(1);

  if (digitalRead(P_LORA_KCT8103L_PA_CSD) == HIGH) {
    // V4.3 KCT8103L has an external pull-up on CSD, so this detection is
    // positive and electrically distinguishable.
    fem_type = KCT8103L_PA;
    pinMode(P_LORA_KCT8103L_PA_CSD, OUTPUT);
    digitalWrite(P_LORA_KCT8103L_PA_CSD, HIGH);
    gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_KCT8103L_PA_CTX));
    pinMode(P_LORA_KCT8103L_PA_CTX, OUTPUT);
    digitalWrite(P_LORA_KCT8103L_PA_CTX, lna_enabled ? LOW : HIGH);
    lna_can_control = true;
  } else {
    // V4.2 GC1109 is the documented fallback for the high-power HF build.
    // Unlike the KCT path it has no positive identity signal, so diagnostics
    // report this result as inferred rather than certain.
    fem_type = GC1109_PA;
    pinMode(P_LORA_GC1109_PA_EN, OUTPUT);
    digitalWrite(P_LORA_GC1109_PA_EN, HIGH);
    gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_GC1109_PA_TX_EN));
    pinMode(P_LORA_GC1109_PA_TX_EN, OUTPUT);
    digitalWrite(P_LORA_GC1109_PA_TX_EN, LOW);
  }
#endif
}

void LoRaFEMControl::setSleepModeEnable()
{
  if (fem_type == GC1109_PA) {
    digitalWrite(P_LORA_GC1109_PA_EN, LOW);
    digitalWrite(P_LORA_GC1109_PA_TX_EN, LOW);
  } else if (fem_type == KCT8103L_PA) {
    digitalWrite(P_LORA_KCT8103L_PA_CSD, LOW);
    digitalWrite(P_LORA_KCT8103L_PA_CTX, HIGH);
  }
}

void LoRaFEMControl::setTxModeEnable()
{
  if (fem_type == GC1109_PA) {
    digitalWrite(P_LORA_GC1109_PA_EN, HIGH);
    digitalWrite(P_LORA_GC1109_PA_TX_EN, HIGH);
  } else if (fem_type == KCT8103L_PA) {
    digitalWrite(P_LORA_KCT8103L_PA_CSD, HIGH);
    digitalWrite(P_LORA_KCT8103L_PA_CTX, HIGH);
  }
}

void LoRaFEMControl::setRxModeEnable()
{
  if (fem_type == GC1109_PA) {
    digitalWrite(P_LORA_GC1109_PA_EN, HIGH);
    digitalWrite(P_LORA_GC1109_PA_TX_EN, LOW);
  } else if (fem_type == KCT8103L_PA) {
    digitalWrite(P_LORA_KCT8103L_PA_CSD, HIGH);
    digitalWrite(P_LORA_KCT8103L_PA_CTX, lna_enabled ? LOW : HIGH);
  }
}

void LoRaFEMControl::setRxModeEnableWhenMCUSleep()
{
  if (!isHighPowerPath()) {
    return;
  }

  digitalWrite(P_LORA_PA_POWER, HIGH);
  gpio_hold_en(static_cast<gpio_num_t>(P_LORA_PA_POWER));

  if (fem_type == GC1109_PA) {
    digitalWrite(P_LORA_GC1109_PA_EN, HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(P_LORA_GC1109_PA_EN));
    digitalWrite(P_LORA_GC1109_PA_TX_EN, LOW);
    gpio_hold_en(static_cast<gpio_num_t>(P_LORA_GC1109_PA_TX_EN));
  } else if (fem_type == KCT8103L_PA) {
    digitalWrite(P_LORA_KCT8103L_PA_CSD, HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(P_LORA_KCT8103L_PA_CSD));
    digitalWrite(P_LORA_KCT8103L_PA_CTX, lna_enabled ? LOW : HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(P_LORA_KCT8103L_PA_CTX));
  }
  gpio_deep_sleep_hold_en();
}

void LoRaFEMControl::setLNAEnable(bool enabled)
{
  lna_enabled = enabled;
}
