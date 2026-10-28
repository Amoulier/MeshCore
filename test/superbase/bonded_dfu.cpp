#include "SuperbaseBondedDFU.h"
#include "SuperbaseDFU.h"
#include <cassert>
#include <iostream>

volatile SuperbaseDfuState superbase_dfu_state = SuperbaseDfuState::Running;
uint16_t superbase_dfu_control_handle = 18, superbase_gatt_last_handle = 39;
uint32_t superbase_service_changed_status = UINT32_MAX;
static bool known_loader = false;
static int jumps = 0;
bool superbaseDfuPeerCrcFirst() { return known_loader; }
bool superbaseDfuPrepareUsb() { ++mock_usb_prepare_calls; return mock_usb_prepare_ok; }
void superbaseDfuJumpToBootloader(uint16_t handle) {
  assert(handle==1 && mock_indication==std::vector<uint8_t>({0x20,1,1}));
  assert(mock_usb_prepare_ok && mock_usb_prepare_calls>0);
  ++jumps;
}

int main() {
  SuperbaseBondedDFU entry;
  assert(entry.begin()==ERROR_NONE && mock_service_begins==0);
  known_loader=true;
  mock_sc_confirm_cb = SuperbaseBondedDFU::serviceChangedConfirmed;
  assert(entry.begin()==ERROR_NONE && mock_service_begins==1 && entry.uuid==0xFE59);
  assert(mock_bond_control && mock_bond_control->uuid[12]==4);
  assert(mock_bond_control->properties==(CHR_PROPS_WRITE|CHR_PROPS_INDICATE));
  ble_gatts_evt_write_t request;
  auto send = [&]() { mock_bond_control->invoke(request); };
  auto rejected = [&](int status) {
    send(); assert(mock_authorize_error==status && superbase_dfu_state==SuperbaseDfuState::Running);
    assert(jumps==0 && mock_indications==0 && mock_usb_prepare_calls==0);
  };
  request.len=2; rejected(BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED);
  request.len=1; request.data[0]=2; rejected(BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED);
  request.data[0]=1; request.offset=1; rejected(BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED);
  request.offset=0; request.op=2; rejected(BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED);
  request.op=BLE_GATTS_OP_WRITE_REQ; request.handle=2; rejected(BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED);
  request.handle=1;
  Bluefruit.peer.secure=false; rejected(BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION);
  Bluefruit.peer.secure=true; Bluefruit.peer.bond=false; rejected(BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION);
  Bluefruit.peer.bond=true; Bluefruit.peer.stored_key=false; rejected(BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION);
  Bluefruit.peer.stored_key=true; Bluefruit.peer.active=false; rejected(BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION);
  Bluefruit.peer.active=true; mock_bond_control->indications_subscribed=false;
  rejected(BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR);
  mock_bond_control->indications_subscribed=true;
  mock_authorize_result=1; send(); assert(superbase_dfu_state==SuperbaseDfuState::Running);
  mock_authorize_result=0;

  send(); assert(superbase_dfu_state==SuperbaseDfuState::Requested && mock_authorize_error==0);
  SuperbaseBondedDFU::loop(); assert(mock_indications==0 && jumps==0);
  send(); assert(mock_authorize_error==BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES);
  superbase_dfu_state=SuperbaseDfuState::Rejected;
  SuperbaseBondedDFU::loop();
  assert(mock_indication==std::vector<uint8_t>({0x20,1,4}) && jumps==0 && mock_usb_prepare_calls==0);
  assert(superbase_dfu_state==SuperbaseDfuState::Running);

  send(); superbase_dfu_state=SuperbaseDfuState::Ready;
  Bluefruit.peer.active=false; int prior=mock_indications;
  SuperbaseBondedDFU::loop(); assert(mock_indications==prior && jumps==0);
  Bluefruit.peer.active=true;

  send(); superbase_dfu_state=SuperbaseDfuState::Ready; mock_indicate_ok=false;
  SuperbaseBondedDFU::loop(); assert(jumps==0 && mock_usb_prepare_calls==0);
  mock_indicate_ok=true;
  send(); superbase_dfu_state=SuperbaseDfuState::Ready; mock_sc_result=1;
  SuperbaseBondedDFU::loop(); assert(jumps==0 && mock_usb_prepare_calls==0);
  mock_sc_result=0;
  send(); superbase_dfu_state=SuperbaseDfuState::Ready; mock_sc_confirm=false;
  const uint32_t before=millis();
  SuperbaseBondedDFU::loop(); assert(jumps==0 && mock_usb_prepare_calls==0 && millis()-before>=1000);
  mock_sc_confirm=true;
  send(); superbase_dfu_state=SuperbaseDfuState::Ready; mock_usb_prepare_ok=false;
  SuperbaseBondedDFU::loop(); assert(jumps==0 && mock_usb_prepare_calls==1);
  mock_usb_prepare_ok=true;

  send(); superbase_dfu_state=SuperbaseDfuState::Ready;
  SuperbaseBondedDFU::loop(); assert(jumps==1 && mock_usb_prepare_calls==2);
  assert(superbase_dfu_state==SuperbaseDfuState::Running);
  SuperbaseBondedDFU::loop(); assert(jumps==1);
  std::cout << "PASS: bonded Nordic entry, encrypted persistent peer, indication acknowledgement, storage/USB failure gating and legacy handoff\n";
}
