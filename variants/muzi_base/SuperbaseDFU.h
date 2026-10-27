#pragma once

#include <bluefruit.h>

// The BLE task waits for the application loop to finish its storage operations.
enum class SuperbaseDfuState : uint8_t { Running, Requested, Ready, Rejected };
extern volatile SuperbaseDfuState superbase_dfu_state;

class SuperbaseDFU : public BLEDfu {
  static BLECharacteristic::write_authorize_cb_t _original;
  static void authorize(uint16_t handle, BLECharacteristic* characteristic, ble_gatts_evt_write_t* request);
public:
  err_t begin() override;
};
