#pragma once
#include <Arduino.h>
using err_t = int;
#define ERROR_NONE 0
#define BLE_GATTS_OP_WRITE_REQ 1
#define BLE_GATTS_AUTHORIZE_TYPE_WRITE 1
#define BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED 6
#define BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES 17
struct ble_gatts_evt_write_t { uint16_t len = 1; uint8_t op = 1; uint8_t data[1] = {1}; };
struct ble_gatts_rw_authorize_reply_params_t { int type; struct { int gatt_status; } params_write_unused; struct { struct { int gatt_status; } write; } params; };
inline int mock_authorize_error = 0, mock_dfu_calls = 0;
inline void sd_ble_gatts_rw_authorize_reply(uint16_t, ble_gatts_rw_authorize_reply_params_t* reply) {
  mock_authorize_error = reply->params.write.gatt_status;
}
class BLECharacteristic {
public:
  using write_authorize_cb_t = void (*)(uint16_t, BLECharacteristic*, ble_gatts_evt_write_t*);
protected:
  write_authorize_cb_t _wr_authorize_cb = nullptr;
public:
  bool subscribed = true;
  bool notifyEnabled(uint16_t) { return subscribed; }
  void setWriteAuthorizeCallback(write_authorize_cb_t cb) { _wr_authorize_cb = cb; }
  void invoke(ble_gatts_evt_write_t& request) { _wr_authorize_cb(1, this, &request); }
};
inline BLECharacteristic* mock_dfu_control = nullptr;
class BLEDfu {
protected:
  BLECharacteristic _chr_control;
public:
  virtual err_t begin() {
    _chr_control.setWriteAuthorizeCallback([](uint16_t, BLECharacteristic*, ble_gatts_evt_write_t*) { mock_dfu_calls++; });
    mock_dfu_control = &_chr_control;
    return ERROR_NONE;
  }
};
