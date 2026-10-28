#pragma once
#include <Arduino.h>
#include <cassert>
using err_t = int;
#define ERROR_NONE 0
#define BLE_GATTS_OP_WRITE_REQ 1
#define BLE_GATTS_AUTHORIZE_TYPE_WRITE 1
#define BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED 6
#define BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES 17
#define BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR 253
#define BLE_GATT_STATUS_SUCCESS 0
#define NRF_SUCCESS 0
struct ble_gatts_evt_write_t { uint16_t len = 1; uint8_t op = 1; uint8_t data[3] = {1,4,0}; uint16_t handle = 1; uint16_t offset = 0; };
struct ble_gatts_rw_authorize_reply_params_t { int type; struct { int gatt_status; } params_write_unused; struct { struct { int gatt_status; } write; } params; };
inline int mock_authorize_error = 0, mock_dfu_calls = 0;
inline int mock_authorize_result = 0;
inline int mock_usb_prepare_calls = 0;
inline bool mock_usb_prepare_ok = true;
inline int sd_ble_gatts_rw_authorize_reply(uint16_t, ble_gatts_rw_authorize_reply_params_t* reply) {
  mock_authorize_error = reply->params.write.gatt_status;
  return mock_authorize_result;
}
class BLECharacteristic {
public:
  using write_authorize_cb_t = void (*)(uint16_t, BLECharacteristic*, ble_gatts_evt_write_t*);
protected:
  write_authorize_cb_t _wr_authorize_cb = nullptr;
public:
  bool subscribed = true;
  struct Handles { uint16_t value_handle = 1; };
  Handles handles() { return {}; }
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
    _chr_control.setWriteAuthorizeCallback([](uint16_t handle, BLECharacteristic*, ble_gatts_evt_write_t*) {
      assert(mock_usb_prepare_calls > 0);
      ++mock_dfu_calls;
      ble_gatts_rw_authorize_reply_params_t reply = {};
      reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
      reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
      sd_ble_gatts_rw_authorize_reply(handle, &reply);
    });
    mock_dfu_control = &_chr_control;
    return ERROR_NONE;
  }
};

#ifdef NRF52_PLATFORM
inline bool mock_usb_events[3] = {true, true, true};
inline int mock_usb_fail_event = -1, mock_usb_removals = 0;
inline int mock_usb_enable(int event, bool enable) {
  if (!enable && event == mock_usb_fail_event) return 1;
  mock_usb_events[event] = enable;
  return NRF_SUCCESS;
}
inline int sd_power_usbdetected_enable(bool enable) { return mock_usb_enable(0, enable); }
inline int sd_power_usbpwrrdy_enable(bool enable) { return mock_usb_enable(1, enable); }
inline int sd_power_usbremoved_enable(bool enable) { return mock_usb_enable(2, enable); }
#endif
