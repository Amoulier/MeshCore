#pragma once
#include <bluefruit.h>
#include <atomic>

// Nordic's bonded Buttonless entry preserves the phone's bond automatically.
// The installed bootloader still receives its existing encrypted Legacy DFU.
class SuperbaseBondedDFU : public BLEService {
  BLECharacteristic _control;
  std::atomic<uint16_t> _request{BLE_CONN_HANDLE_INVALID};
  std::atomic<uint16_t> _cacheConfirmed{BLE_CONN_HANDLE_INVALID};
  static SuperbaseBondedDFU* _instance;
  static void authorize(uint16_t handle, BLECharacteristic* characteristic, ble_gatts_evt_write_t* request);
  void process();
public:
  SuperbaseBondedDFU();
  err_t begin() override;
  static void loop();
  static void serviceChangedConfirmed(uint16_t handle);
};

// Extracted from the pinned Bluefruit implementation into the build-local copy.
void superbaseDfuJumpToBootloader(uint16_t handle);
