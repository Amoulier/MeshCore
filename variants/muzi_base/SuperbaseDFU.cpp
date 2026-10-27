#ifdef MUZI_BASE_SUPERIO
#include "SuperbaseDFU.h"

volatile SuperbaseDfuState superbase_dfu_state = SuperbaseDfuState::Running;
BLECharacteristic::write_authorize_cb_t SuperbaseDFU::_original = nullptr;

err_t SuperbaseDFU::begin() {
  err_t result = BLEDfu::begin();
  if (result != ERROR_NONE) return result;
  struct CallbackAccess : BLECharacteristic {
    static write_authorize_cb_t original(BLECharacteristic& characteristic) {
      return characteristic.*(&CallbackAccess::_wr_authorize_cb);
    }
  };
  _original = CallbackAccess::original(_chr_control);
  _chr_control.setWriteAuthorizeCallback(authorize);
  return result;
}

void SuperbaseDFU::authorize(uint16_t handle, BLECharacteristic* characteristic, ble_gatts_evt_write_t* request) {
  if (request->len == 0 || request->op != BLE_GATTS_OP_WRITE_REQ || !_original) {
    ble_gatts_rw_authorize_reply_params_t reply = {};
    reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED;
    sd_ble_gatts_rw_authorize_reply(handle, &reply);
    return;
  }
  if (request->data[0] == 1 && characteristic->notifyEnabled(handle)) {
    superbase_dfu_state = SuperbaseDfuState::Requested;
    uint32_t started = millis();
    while (superbase_dfu_state == SuperbaseDfuState::Requested && uint32_t(millis() - started) < 3000) delay(1);
    if (superbase_dfu_state != SuperbaseDfuState::Ready) {
      superbase_dfu_state = SuperbaseDfuState::Running;
      ble_gatts_rw_authorize_reply_params_t reply = {};
      reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
      reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES;
      sd_ble_gatts_rw_authorize_reply(handle, &reply);
      return;
    }
  }
  _original(handle, characteristic, request);
  superbase_dfu_state = SuperbaseDfuState::Running;
}
#endif
