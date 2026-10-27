#include <Arduino.h>
#include <Wire.h>

#include "MuziBaseBoard.h"
#include <flash/flash_nrf5x.h>
#ifdef MUZI_BASE_SUPERIO
#include <ICM_20948.h>
#endif

#ifdef NRF52_POWER_MANAGEMENT
const PowerMgtConfig power_config = {
  .lpcomp_ain_channel = PWRMGT_LPCOMP_AIN,
  .lpcomp_refsel = PWRMGT_LPCOMP_REFSEL,
  .voltage_bootlock = PWRMGT_VOLTAGE_BOOTLOCK
};

void MuziBaseBoard::initiateShutdown(uint8_t reason) {
  // Disable LoRa module power before shutdown
  if (reason == SHUTDOWN_REASON_LOW_VOLTAGE ||
      reason == SHUTDOWN_REASON_BOOT_PROTECT) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }

  enterSystemOff(reason);
}
#endif // NRF52_POWER_MANAGEMENT

void MuziBaseBoard::begin() {
  NRF52BoardDCDC::begin();
  pinMode(PIN_VBAT_READ, INPUT);
  pinMode(PIN_BATTERY_CHARGING, INPUT);
  pinMode(PIN_CHARGER_FAULT, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // output latches default to HIGH, so pull these low right after enabling
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  // gps power is driven by the sensor manager (mode switch). off to start.
  pinMode(PIN_GPS_EN, OUTPUT);
  digitalWrite(PIN_GPS_EN, LOW);
  // 12V rail is only needed for the superIO display
  pinMode(SCREEN_12V_ENABLE, OUTPUT);
#ifdef MUZI_BASE_SUPERIO
  digitalWrite(SCREEN_12V_ENABLE, HIGH); // Enable 12V power for SH1107 display
  delay(250);
#else
  digitalWrite(SCREEN_12V_ENABLE, LOW);
#endif
  Wire.begin();
#ifdef MUZI_BASE_SUPERIO
  // Companion does not consume compass data, so keep both sensors asleep.
  Wire1.begin();
  ICM_20948_I2C imu;
  for (uint8_t ad0 = 0; ad0 < 2; ad0++) {
    imu.begin(Wire1, ad0);
    if (imu.status != ICM_20948_Stat_Ok) continue;
    AK09916_CNTL2_Reg_t mode = {};
    mode.MODE = AK09916_mode_power_down;
    imu.writeMag(AK09916_REG_CNTL2, reinterpret_cast<uint8_t*>(&mode));
    imu.sleep(true);
    break;
  }
  Wire1.end();
#endif
  // delay(1000); // wait for display to initialize. otherwise it doesn't come up on boot.

#ifdef PIN_USER_BTN
  pinMode(PIN_USER_BTN, INPUT_PULLUP);
#endif
  pinMode(PIN_BUTTON1, INPUT_PULLUP);
  pinMode(PIN_BUTTON2, INPUT_PULLUP);
  pinMode(PIN_BUTTON3, INPUT_PULLUP);
  pinMode(PIN_BUTTON4, INPUT_PULLUP);
  pinMode(PIN_BUTTON5, INPUT_PULLUP);
  pinMode(PIN_BUTTON6, INPUT_PULLUP);

// #if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
//   Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
// #endif
#ifdef NRF52_POWER_MANAGEMENT
  checkBootVoltage(&power_config);
#endif
}

void MuziBaseBoard::shutdownPeripherals() {
  NRF52Board::shutdownPeripherals();
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(SCREEN_12V_ENABLE, LOW);
  digitalWrite(PIN_GPS_EN, LOW);
  flash_nrf5x_flush();
}

void MuziBaseBoard::reboot() {
  if (prepareShutdown && !prepareShutdown()) return;
  flash_nrf5x_flush();
  NRF52Board::reboot();
}

void MuziBaseBoard::powerOff() {
  if (prepareShutdown && !prepareShutdown()) return;
  NRF52Board::powerOff();
}
