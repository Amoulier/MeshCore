#include <cassert>
#include <iostream>
#include <string>
#include <map>
#include <helpers/ui/SuperbaseSupport.h>
#include <helpers/ui/BQ25185Inputs.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include "SuperbaseButton.h"
#include <helpers/ui/buzzer.h>
#include "SuperbaseDFU.h"
#include <helpers/ArduinoSerialInterface.h>
#include <helpers/MultiSerialInterface.h>

struct BatteryService {
  int writes = 0, notifications = 0;
  bool writable = true, subscribed = false;
  uint8_t value = 0;
  bool write(uint8_t v) { writes++; value = v; return writable; }
  bool notify(uint16_t, uint8_t) { notifications++; return subscribed; }
};

struct FakeFS {
  std::map<std::string, std::string> files;
  bool can_open = true, can_rename = true, can_remove = true;
  struct File {
    FakeFS* fs;
    explicit operator bool() const { return fs->can_open; }
    void flush() {}
    void close() {}
  };
  bool exists(const char* path) { return files.count(path); }
  bool remove(const char* path) { if (!can_remove) return false; files.erase(path); return true; }
  File open(const char* path, uint8_t) { if (can_open) files[path] = "partial"; return {this}; }
  bool rename(const char* from, const char* to) {
    if (!can_rename) return false;
    files[to] = files[from]; files.erase(from); return true;
  }
};
struct FakePrefs {
  bool complete = true;
  bool saveSerial(FakeFS::File& file) {
    if (complete) file.fs->files["/prefs.tmp"] = "new preferences";
    return complete;
  }
};

class TestRadio : public RadioLibWrapper {
public:
  int recoveries = 0;
  bool recovery_ok = false;
  TestRadio(PhysicalLayer& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) {}
  bool recoverReceive() override { recoveries++; return recovery_ok; }
  bool isReceivingPacket() override { return false; }
  void setParams(float, float, uint8_t, uint8_t) override {}
  float getCurrentRSSI() override { return -100; }
};

void batteryTests() {
  superbase::BatteryDelivery delivery;
  BatteryService service;
  service.writable = false;
  delivery.update(service, 50, true, 1);
  assert(service.notifications == 0);
  service.writable = true;
  delivery.update(service, 50, true, 1);
  delivery.update(service, 50, true, 1);
  assert(service.writes == 2 && service.notifications == 2);
  service.subscribed = true;
  delivery.update(service, 50, true, 1);
  delivery.update(service, 50, true, 1);
  assert(service.notifications == 3);
  delivery.disconnected();
  delivery.update(service, 50, true, 2);
  assert(service.notifications == 4);
  // A bonded host may consume the reconnect notification before attaching its
  // listener. A new subscription must re-send the same percentage and retry.
  service.subscribed = false;
  delivery.subscribed();
  delivery.update(service, 50, true, 2);
  assert(service.notifications == 5);
  service.subscribed = true;
  delivery.update(service, 50, true, 2);
  delivery.update(service, 50, true, 2);
  assert(service.notifications == 6);
  delivery.subscribed();
  delivery.update(service, 50, true, 2);
  assert(service.notifications == 7);
  delivery.update(service, 255, true, 2);
  assert(service.value == 100);
  assert(superbase::batteryPercent(1000) == 0 && superbase::batteryPercent(5000) == 100);
  assert(superbase::batteryPercent(3600) == 50);
}

void policyTests() {
  using namespace superbase;
  for (int s1 = 0; s1 < 2; s1++) for (int s2 = 0; s2 < 2; s2++)
    assert(chargeState(false, s1, s2) == ChargeState::Battery);
  assert(chargeState(true, 1, 0) == ChargeState::Charging);
  assert(chargeState(true, 1, 1) == ChargeState::Idle);
  assert(chargeState(true, 0, 1) == ChargeState::RecoverableFault);
  assert(chargeState(true, 0, 0) == ChargeState::LatchedFault);
  assert(allowSound(DirectOnly, true, true));
  assert(!allowSound(DirectOnly, true, false));
  assert(!allowSound(DirectOnly, false));
  assert(allowSound(Notifications, true) && !allowSound(Notifications, false));
  assert(allowSound(System, false) && !allowSound(System, true));
  assert(!allowSound(Silent, true, true));
  RetryTimer timer;
  assert(timer.ready(0, 1000)); timer.attempted(0);
  assert(!timer.ready(999, 1000) && timer.ready(1000, 1000));
  timer.attempted(UINT32_MAX - 500);
  assert(!timer.ready(100, 1000) && timer.ready(500, 1000));
}

void chargerInputTests() {
  using namespace superbase;
  BQ25185Inputs charger(27, 34);
  mock_open_drain[27] = true;
  mock_pins[27] = HIGH; // Released fault output, with no external pull-up.
  mock_pins[34] = LOW;  // Charging.
  pinMode(27, INPUT);
  assert(charger.read(true) == ChargeState::LatchedFault);
  charger.begin();
  assert(charger.read(true) == ChargeState::Charging);
  mock_pins[34] = HIGH;
  assert(charger.read(true) == ChargeState::Idle);
  mock_pins[27] = LOW; // Real fault must win over the weak pull-up.
  assert(charger.read(true) == ChargeState::RecoverableFault);
  mock_pins[34] = LOW;
  assert(charger.read(true) == ChargeState::LatchedFault);
  assert(charger.read(false) == ChargeState::Battery);
  mock_open_drain[27] = false;
}

void persistenceTests() {
  FakeFS fs; FakePrefs prefs;
  fs.files["/prefs.json"] = "old preferences";
  fs.can_open = false;
  assert(!superbase::savePreferences(fs, prefs, 1));
  assert(fs.files["/prefs.json"] == "old preferences");
  fs.can_open = true; prefs.complete = false;
  assert(!superbase::savePreferences(fs, prefs, 1));
  assert(fs.files["/prefs.json"] == "old preferences");
  prefs.complete = true; fs.can_rename = false;
  assert(!superbase::savePreferences(fs, prefs, 1));
  assert(fs.files["/prefs.json"] == "old preferences");
  fs.can_rename = true; fs.can_remove = false;
  assert(!superbase::savePreferences(fs, prefs, 1));
  fs.can_remove = true;
  assert(superbase::savePreferences(fs, prefs, 1));
  assert(fs.files["/prefs.json"] == "new preferences" && !fs.exists("/prefs.tmp"));
}

void buttonTests() {
  mock_pins[10] = HIGH;
  SuperbaseButton button(0, 10);
  button.begin();
  auto edge = [&](uint32_t at, bool down) {
    mock_now = at; mock_pins[10] = down ? LOW : HIGH; mock_irqs[10]();
  };
  edge(0, true); edge(35, false); mock_now = 100;
  assert(button.check() == BUTTON_EVENT_CLICK);
  assert(button.check() == BUTTON_EVENT_NONE);
  edge(200, true); edge(203, false); edge(206, true); edge(250, false);
  assert(button.check() == BUTTON_EVENT_CLICK);
  assert(button.check() == BUTTON_EVENT_NONE);
  edge(UINT32_MAX - 20, true); edge(20, false); mock_now = 30;
  assert(button.check() == BUTTON_EVENT_CLICK);
  edge(100, true); mock_now = 1100;
  assert(button.check() == BUTTON_EVENT_LONG_PRESS);
  assert(button.check() == BUTTON_EVENT_NONE);
  edge(1200, false); assert(button.check() == BUTTON_EVENT_NONE);
  // Polling fallback when no hardware interrupt is delivered.
  mock_now = 1300; mock_pins[10] = LOW; assert(button.check() == 0);
  mock_now = 1350; mock_pins[10] = HIGH; assert(button.check() == BUTTON_EVENT_CLICK);
  mock_pins[15] = HIGH;
  SuperbaseButton back(5, 15, 1000, true);
  back.begin();
  auto tap = [&](uint32_t at) {
    mock_now = at; mock_pins[15] = LOW; mock_irqs[15]();
    mock_now = at + 25; mock_pins[15] = HIGH; mock_irqs[15]();
  };
  tap(2000); tap(2100); tap(2200);
  assert(back.check() == BUTTON_EVENT_TRIPLE_CLICK);
  tap(3000); tap(4000); tap(5000);
  assert(back.check() == BUTTON_EVENT_CLICK);
  assert(back.check() == BUTTON_EVENT_CLICK);
  assert(back.check() == BUTTON_EVENT_NONE);
  mock_now = 5400;
  assert(back.check() == BUTTON_EVENT_CLICK);
  assert(back.check() == BUTTON_EVENT_NONE);
}

void radioTests() {
  mock_now = 0;
  PhysicalLayer chip; mesh::MainBoard board; TestRadio radio(chip, board);
  radio.begin(); chip.receive_result = -1;
  uint8_t data[8] = {};
  radio.recvRaw(data, 8); radio.recvRaw(data, 8);
  assert(chip.receive_calls == 1);
  mock_now = 1000; radio.recvRaw(data, 8);
  mock_now = 2000; radio.recvRaw(data, 8);
  mock_now = 3000; radio.recvRaw(data, 8); radio.recvRaw(data, 8);
  assert(radio.recoveries == 1 && chip.receive_calls == 3);
  radio.recovery_ok = true; chip.receive_result = 0;
  mock_now = 4000; radio.recvRaw(data, 8);
  assert(radio.isInRecvMode() && radio.recoveries == 2);
  assert(radio.startSendRaw(data, 1));
  assert(!radio.startSendRaw(data, 1));
  int received = chip.receive_calls;
  radio.recvRaw(data, 8); radio.resetAGC();
  assert(chip.receive_calls == received && chip.sleep_calls == 0);
  chip.irq(); assert(radio.isSendComplete()); radio.onSendFinished();
  radio.recvRaw(data, 8);
  assert(chip.receive_calls == received + 1 && board.before == 1 && board.after == 1);
}

void soundTests() {
  genericBuzzer first, second;
  first.begin(); second.begin();
  char text[] = "message:d=4,o=5,b=120:c";
  first.play(text); text[0] = 'X';
  assert(first.isPlaying() && !second.isPlaying() && rtttl::melody[0] == 'm');
  int starts = rtttl::starts;
  second.play("other:d=4,o=5,b=120:d");
  second.quiet(true); second.loop();
  assert(first.isPlaying() && rtttl::starts == starts && rtttl::advances == 0);
  first.quiet(true);
  assert(!first.isPlaying() && !rtttl::playing);
  second.quiet(false); second.play("other:d=4,o=5,b=120:d");
  assert(second.isPlaying()); second.quiet(true);
}

bool superbaseDfuPrepareUsb() {
  assert(superbase_dfu_state == SuperbaseDfuState::Ready);
  ++mock_usb_prepare_calls;
  return mock_usb_prepare_ok;
}

void dfuTests() {
  SuperbaseDFU dfu;
  assert(dfu.begin() == ERROR_NONE);
  ble_gatts_evt_write_t request;
  // Exercise exactly the production path, with Nordic's real 01 04 payload.
  for (uint16_t length : {2, 1}) {
    request.len = length;
    mock_delay_hook = []() { superbase_dfu_state = SuperbaseDfuState::Ready; };
    const int prior = mock_dfu_calls;
    mock_dfu_control->invoke(request);
    mock_delay_hook = nullptr;
    assert(mock_dfu_calls == prior + 1);
    assert(mock_authorize_error == BLE_GATT_STATUS_SUCCESS);
    assert(superbase_dfu_state == SuperbaseDfuState::Running);
  }
  request.len = 2;
  const int prior_calls = mock_dfu_calls, prior_usb = mock_usb_prepare_calls;
  mock_delay_hook = []() { superbase_dfu_state = SuperbaseDfuState::Rejected; };
  mock_dfu_control->invoke(request);
  assert(mock_authorize_error == BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES);
  mock_delay_hook = nullptr;
  mock_now = UINT32_MAX - 100; // Storage timeout across millis rollover.
  mock_dfu_control->invoke(request);
  assert(mock_authorize_error == BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES);
  assert(superbase_dfu_state == SuperbaseDfuState::Running);
  assert(mock_dfu_calls == prior_calls && mock_usb_prepare_calls == prior_usb);

  mock_dfu_control->subscribed = false;
  mock_dfu_control->invoke(request);
  assert(mock_authorize_error == BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR);
  mock_dfu_control->subscribed = true;
  superbase_dfu_state = SuperbaseDfuState::Ready; // Another transition owns storage.
  mock_dfu_control->invoke(request);
  assert(mock_authorize_error == BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES);
  assert(superbase_dfu_state == SuperbaseDfuState::Ready);
  superbase_dfu_state = SuperbaseDfuState::Running;

  auto rejected = [&]() {
    mock_dfu_control->invoke(request);
    assert(mock_authorize_error == BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED);
    assert(mock_dfu_calls == prior_calls && mock_usb_prepare_calls == prior_usb);
  };
  request.len = 0; rejected();
  request.len = 3; rejected();
  request.len = 2; request.data[1] = 2; rejected();
  request.data[1] = 4; request.data[0] = 2; rejected();
  request.data[0] = 1; request.handle = 2; rejected();
  request.handle = 1; request.offset = 1; rejected();
  request.offset = 0; request.op = 2; rejected();
  request.op = BLE_GATTS_OP_WRITE_REQ;

  // USB failures cannot hand off to a bootloader with an active controller.
  mock_usb_prepare_ok = false;
  mock_delay_hook = []() { superbase_dfu_state = SuperbaseDfuState::Ready; };
  mock_dfu_control->invoke(request);
  mock_delay_hook = nullptr;
  assert(mock_dfu_calls == prior_calls && mock_usb_prepare_calls == prior_usb + 1);
  assert(mock_authorize_error == BLE_GATT_STATUS_ATTERR_INSUF_RESOURCES);
  assert(superbase_dfu_state == SuperbaseDfuState::Running);
  mock_usb_prepare_ok = true;
}

class TestBLEInterface : public BaseSerialInterface {
public:
  bool enabled = false, connected = false;
  std::vector<uint8_t> incoming, outgoing;
  void enable() override { enabled = true; }
  void disable() override { enabled = false; }
  bool isEnabled() const override { return enabled; }
  bool isConnected() const override { return enabled && connected; }
  bool isWriteBusy() const override { return false; }
  size_t writeFrame(const uint8_t* data, size_t len) override {
    outgoing.insert(outgoing.end(), data, data+len); return len;
  }
  size_t checkRecvFrame(uint8_t* dest) override {
    size_t len = incoming.size();
    std::copy(incoming.begin(), incoming.end(), dest); incoming.clear(); return len;
  }
};

void usbTransportTests() {
  ArduinoSerialInterface usb;
  usb.begin(Serial); usb.enable();
  assert(!usb.isConnected());
  Serial.connected = true;
  assert(usb.isConnected());
  uint8_t frame[176] = {};
  Serial.input = {'<',3,0,66};
  assert(usb.checkRecvFrame(frame) == 0);
  Serial.input.push_back('v'); Serial.input.push_back('e');
  assert(usb.checkRecvFrame(frame) == 3 && frame[0] == 66 && frame[2] == 'e');
  uint8_t reply[] = {29,'O','K'};
  assert(usb.writeFrame(reply, 3) == 3);
  assert(Serial.output == std::vector<uint8_t>({'>',3,0,29,'O','K'}));
  Serial.output.clear();
  TestBLEInterface ble;
  MultiSerialInterface manager;
  manager.addInterface(InterfaceType::Bluetooth, &ble);
  manager.addInterface(InterfaceType::USB, &usb);
  manager.enable(); ble.connected = true;
  Serial.input.insert(Serial.input.end(), {'<',1,0,20});
  assert(manager.checkRecvFrame(frame) == 1 && frame[0] == 20);
  assert(manager.writeFrame(reply, 3) == 3 && ble.outgoing.empty());
  assert(Serial.output.size() == 6);
  Serial.output.clear();
  uint8_t push[] = {0x83,1};
  assert(manager.writeFrame(push, 2) == 2 && ble.outgoing.size() == 2 && Serial.output.size() == 5);
  Serial.connected = false;
  ble.incoming = {20};
  assert(manager.checkRecvFrame(frame) == 1);
  assert(manager.writeFrame(reply, 3) == 3 && ble.outgoing.size() == 5);
  ble.connected = false;
  assert(!manager.isConnected());
}

int main() {
  batteryTests(); policyTests(); chargerInputTests(); persistenceTests(); buttonTests(); radioTests(); soundTests(); dfuTests(); usbTransportTests();
  std::cout << "PASS: battery delivery, charger, audio ownership/policy, safe preferences, navigation, RX recovery, TX protection, DFU gating and USB/BLE reply routing\n";
}
