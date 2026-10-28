#pragma once
#include <Arduino.h>
#include <cassert>
#include <array>
#include <vector>
using err_t = int;
#define ERROR_NONE 0
#define BLE_GATTS_OP_WRITE_REQ 1
#define BLE_GATTS_AUTHORIZE_TYPE_WRITE 1
#define BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED 6
#define BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES 17
#define BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR 253
#define BLE_GATT_STATUS_SUCCESS 0
#define NRF_SUCCESS 0
#define BLE_CONN_HANDLE_INVALID 0xFFFF
#define BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION 5
#define CHR_PROPS_WRITE 0x08
#define CHR_PROPS_INDICATE 0x20
#define SECMODE_ENC_WITH_MITM 3
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
class BLECharacteristic;
inline BLECharacteristic* mock_bond_control = nullptr;
inline bool mock_indicate_ok = true;
inline std::vector<uint8_t> mock_indication;
inline int mock_indications = 0, mock_service_begins = 0;
inline int mock_sc_result = 0;
inline bool mock_sc_confirm = true;
inline void (*mock_sc_confirm_cb)(uint16_t) = nullptr;
inline int sd_ble_gatts_service_changed(uint16_t handle, uint16_t first, uint16_t last) {
  assert(first == 14 && last == 39);
  if (mock_sc_result == NRF_SUCCESS && mock_sc_confirm && mock_sc_confirm_cb) mock_sc_confirm_cb(handle);
  return mock_sc_result;
}
class BLEService {
public:
  uint16_t uuid;
  explicit BLEService(uint16_t value) : uuid(value) {}
  virtual err_t begin() { ++mock_service_begins; return ERROR_NONE; }
  void setPermission(int, int) {}
};
struct bond_keys_t { uint8_t unused = 0; };
class BLEConnection {
public:
  bool active = true, secure = true, bond = true, stored_key = true;
  bool connected() { return active; }
  bool secured() { return secure; }
  bool bonded() { return bond; }
  bool loadBondKey(bond_keys_t*) { return stored_key; }
};
inline struct MockBluefruit {
  BLEConnection peer;
  BLEConnection* Connection(uint16_t handle) { return handle == 1 ? &peer : nullptr; }
} Bluefruit;
class BLECharacteristic {
public:
  using write_authorize_cb_t = void (*)(uint16_t, BLECharacteristic*, ble_gatts_evt_write_t*);
protected:
  write_authorize_cb_t _wr_authorize_cb = nullptr;
public:
  std::array<uint8_t,16> uuid = {};
  int properties = 0;
  BLECharacteristic() = default;
  explicit BLECharacteristic(const uint8_t* value) { for (int i=0;i<16;++i) uuid[i]=value[i]; }
  err_t begin() { mock_bond_control = this; return ERROR_NONE; }
  void setProperties(int value) { properties = value; }
  void setPermission(int, int) {}
  void setMaxLen(int) {}
  bool subscribed = true;
  bool indications_subscribed = true;
  struct Handles { uint16_t value_handle = 1; };
  Handles handles() { return {}; }
  bool notifyEnabled(uint16_t) { return subscribed; }
  bool indicateEnabled(uint16_t) { return indications_subscribed; }
  bool indicate(uint16_t, const void* data, uint16_t length) {
    ++mock_indications;
    const auto* bytes = static_cast<const uint8_t*>(data);
    mock_indication.assign(bytes, bytes+length);
    return mock_indicate_ok;
  }
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
