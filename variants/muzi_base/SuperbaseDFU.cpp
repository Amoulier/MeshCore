#ifdef MUZI_BASE_SUPERIO
#include "SuperbaseDFU.h"

volatile SuperbaseDfuState superbase_dfu_state = SuperbaseDfuState::Running;
uint16_t superbase_dfu_control_handle = 0;
uint16_t superbase_gatt_last_handle = 0;
uint32_t superbase_service_changed_status = UINT32_MAX;
BLECharacteristic::write_authorize_cb_t SuperbaseDFU::_original = nullptr;

#ifdef NRF52_PLATFORM
void superbaseDfuCacheGattRange() {
  uint16_t last = superbase_dfu_control_handle;
  ble_uuid_t uuid = {};
  while (last < UINT16_MAX && sd_ble_gatts_attr_get(last + 1, &uuid, nullptr) == NRF_SUCCESS) ++last;
  superbase_gatt_last_handle = last;
}
#endif

err_t SuperbaseDFU::begin() {
  err_t result = BLEDfu::begin();
  if (result != ERROR_NONE) return result;
  superbase_dfu_control_handle = _chr_control.handles().value_handle;
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
#ifdef SUPERBASE_DFU_TRACE
  if (Serial) Serial.printf("SBDFU legacy len=%u\n", request ? request->len : 0);
#endif
  // Nordic LegacyButtonlessDfuImpl sends START_DFU + APPLICATION (01 04).
  // Older clients use START_DFU alone. Validate before waiting for storage.
  if (!request || !characteristic || !_original ||
      request->op != BLE_GATTS_OP_WRITE_REQ || request->offset != 0 ||
      request->handle != characteristic->handles().value_handle ||
      (request->len != 1 && request->len != 2) || request->data[0] != 1 ||
      (request->len == 2 && request->data[1] != 4)) {
    ble_gatts_rw_authorize_reply_params_t reply = {};
    reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED;
    sd_ble_gatts_rw_authorize_reply(handle, &reply);
    return;
  }
  if (!characteristic->notifyEnabled(handle) || superbase_dfu_state != SuperbaseDfuState::Running) {
    ble_gatts_rw_authorize_reply_params_t reply = {};
    reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
    reply.params.write.gatt_status = !characteristic->notifyEnabled(handle)
      ? BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR : BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES;
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
    if (!superbaseDfuPrepareUsb()) {
      superbase_dfu_state = SuperbaseDfuState::Running;
      ble_gatts_rw_authorize_reply_params_t reply = {};
      reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
      reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES;
      sd_ble_gatts_rw_authorize_reply(handle, &reply);
      return;
    }
  }
  // Preserve Bluefruit's ATT reply, bonded-peer handoff and bootloader jump.
  // The SuperBase linker keeps its 0x20007F80 peer window outside application RAM.
  _original(handle, characteristic, request);
  superbase_dfu_state = SuperbaseDfuState::Running;
}
#endif
