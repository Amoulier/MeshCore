#ifdef MUZI_BASE_SUPERIO
#include "SuperbaseBondedDFU.h"
#include "SuperbaseDFU.h"

namespace {
// 8EC90004-F315-4F60-9FB8-838830DAEA50, Bluetooth little-endian UUID.
const uint8_t bonded_control_uuid[16] = {
  0x50,0xEA,0xDA,0x30,0x88,0x83,0xB8,0x9F,0x60,0x4F,0x15,0xF3,0x04,0x00,0xC9,0x8E
};

bool trustedConnection(uint16_t handle) {
  BLEConnection* connection = Bluefruit.Connection(handle);
  if (!connection || !connection->connected() || !connection->secured() || !connection->bonded()) return false;
  // Verify the existing persistent key is available for the original handoff.
  // It is neither exported nor rewritten here.
  bond_keys_t key;
  return connection->loadBondKey(&key);
}
}

SuperbaseBondedDFU* SuperbaseBondedDFU::_instance = nullptr;

SuperbaseBondedDFU::SuperbaseBondedDFU() : BLEService(0xFE59), _control(bonded_control_uuid) {}

err_t SuperbaseBondedDFU::begin() {
  // Do not claim bond sharing for an unrecognized bootloader ABI.
  if (!superbaseDfuPeerCrcFirst()) return ERROR_NONE;
  err_t result = BLEService::begin();
  if (result != ERROR_NONE) return result;
  setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  _control.setProperties(CHR_PROPS_WRITE | CHR_PROPS_INDICATE);
  _control.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  _control.setMaxLen(3);
  _control.setWriteAuthorizeCallback(authorize);
  result = _control.begin();
  if (result == ERROR_NONE) _instance = this;
  return result;
}

void SuperbaseBondedDFU::authorize(uint16_t handle, BLECharacteristic* characteristic, ble_gatts_evt_write_t* request) {
  uint16_t status = BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED;
  if (_instance && characteristic == &_instance->_control && request &&
      request->handle == characteristic->handles().value_handle &&
      request->op == BLE_GATTS_OP_WRITE_REQ && request->offset == 0 && request->len == 1 && request->data[0] == 1) {
    if (!trustedConnection(handle)) status = BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION;
    else if (!characteristic->indicateEnabled(handle)) status = BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR;
    else if (superbase_dfu_state != SuperbaseDfuState::Running || _instance->_request.load() != BLE_CONN_HANDLE_INVALID)
      status = BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES;
    else status = BLE_GATT_STATUS_SUCCESS;
  }
  ble_gatts_rw_authorize_reply_params_t reply = {};
  reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
  reply.params.write.gatt_status = status;
  if (sd_ble_gatts_rw_authorize_reply(handle, &reply) == NRF_SUCCESS && status == BLE_GATT_STATUS_SUCCESS) {
    _instance->_request.store(handle);
    superbase_dfu_state = SuperbaseDfuState::Requested;
  }
}

void SuperbaseBondedDFU::process() {
  const uint16_t handle = _request.load();
  if (handle == BLE_CONN_HANDLE_INVALID ||
      (superbase_dfu_state != SuperbaseDfuState::Ready && superbase_dfu_state != SuperbaseDfuState::Rejected)) return;
  const bool saved = superbase_dfu_state == SuperbaseDfuState::Ready;
  const uint8_t response[] = {0x20, 0x01, uint8_t(saved ? 0x01 : 0x04)};
  // Indications wait for HVC: run on the application task, never the BLE callback.
  // Failed storage, a lost link or missing confirmation must never enter DFU.
  if (trustedConnection(handle) && _control.indicate(handle, response, sizeof(response)) && saved) {
    // The installed Legacy loader cannot invalidate the application's larger
    // GATT table reliably. Clear that cache while this application still owns
    // the valid handles; wait for the peer's confirmation without removing its bond.
    _cacheConfirmed.store(BLE_CONN_HANDLE_INVALID);
    const bool validRange = superbase_dfu_control_handle >= 4 &&
      superbase_gatt_last_handle >= superbase_dfu_control_handle;
    bool cacheReady = false;
    if (validRange) {
      superbase_service_changed_status = sd_ble_gatts_service_changed(handle,
        superbase_dfu_control_handle - 4, superbase_gatt_last_handle);
      if (superbase_service_changed_status == NRF_SUCCESS) {
        const uint32_t started = millis();
        while (_cacheConfirmed.load() != handle && uint32_t(millis()-started) < 1000) delay(1);
        cacheReady = _cacheConfirmed.load() == handle;
      }
    }
    // This is the last fallible step before the original encrypted handoff.
    // A failure leaves the application running, with USB notifications restored
    // by PrepareUsb; the updater cannot report a completed firmware transfer.
    if (cacheReady && trustedConnection(handle) && superbaseDfuPrepareUsb()) superbaseDfuJumpToBootloader(handle);
  }
  _request.store(BLE_CONN_HANDLE_INVALID);
  superbase_dfu_state = SuperbaseDfuState::Running;
}

void SuperbaseBondedDFU::loop() {
  if (_instance) _instance->process();
}

void SuperbaseBondedDFU::serviceChangedConfirmed(uint16_t handle) {
  if (_instance && _instance->_request.load() == handle) _instance->_cacheConfirmed.store(handle);
}
#endif
