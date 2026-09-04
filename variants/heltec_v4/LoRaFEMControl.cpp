#include "LoRaFEMControl.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

void LoRaFEMControl::init()
{
  lna_can_control = false;

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
    fem_type = KCT8103L_PA;
    pinMode(P_LORA_KCT8103L_PA_CSD, OUTPUT);
    digitalWrite(P_LORA_KCT8103L_PA_CSD, HIGH);
    gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_KCT8103L_PA_CTX));
    pinMode(P_LORA_KCT8103L_PA_CTX, OUTPUT);
    digitalWrite(P_LORA_KCT8103L_PA_CTX, lna_enabled ? LOW : HIGH);
    lna_can_control = true;
  } else {
    fem_type = GC1109_PA;
    pinMode(P_LORA_GC1109_PA_EN, OUTPUT);
    digitalWrite(P_LORA_GC1109_PA_EN, HIGH);
    gpio_hold_dis(static_cast<gpio_num_t>(P_LORA_GC1109_PA_TX_EN));
    pinMode(P_LORA_GC1109_PA_TX_EN, OUTPUT);
    digitalWrite(P_LORA_GC1109_PA_TX_EN, LOW);
  }
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
