// Exercises the actual Muzi manager and GPS initialization methods extracted
// from production sources by run.py. Only hardware/dependency calls are mocked.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#define ENV_INCLUDE_GPS 1
#define ENV_SKIP_GPS_DETECT
#define HIGH 1
#define LOW 0
#define INPUT 0
#define PIN_GPS_SWITCH 12
#define PIN_GPS_TX 26
#define PIN_GPS_RX 27
#define PIN_GPS_EN 33
#define BUTTON_EVENT_CLICK 1
#define USER_BTN_POWER_OFF_CLICKS 5
#define MESH_DEBUG_PRINTLN(...) do {} while (0)

static uint32_t now_ms = 0;
static int switch_level = LOW;
static int input_configurations = 0;
static int delayed_ms = 0;
static int power_off_calls = 0;
static unsigned long millis() { return now_ms; }
static void delay(unsigned long duration) { delayed_ms += duration; }
static int digitalRead(int pin) { assert(pin == PIN_GPS_SWITCH); return switch_level; }
static void pinMode(int pin, int mode) {
  assert(pin == PIN_GPS_SWITCH && mode == INPUT);
  input_configurations++;
}
static void muziPowerOff() { power_off_calls++; }

struct MockSerial {
  int pin_calls = 0, begin_calls = 0, availability_calls = 0;
  unsigned long baud = 0;
  void setPins(int tx, int rx) {
    assert(tx == PIN_GPS_TX && rx == PIN_GPS_RX);
    pin_calls++;
  }
  void begin(unsigned long value) { baud = value; begin_calls++; }
  int available() { availability_calls++; return 0; }
} Serial1;

struct MockButton {
  int begin_calls = 0;
  void begin() { begin_calls++; }
  int check() { return 0; }
} pwr_btn;

struct LocationProvider {
  int begin_calls = 0, reset_calls = 0, stop_calls = 0;
  bool enabled = false;
  void begin() { begin_calls++; enabled = true; }
  void reset() { reset_calls++; }
  void stop() { stop_calls++; enabled = false; }
};

class EnvironmentSensorManager {
protected:
  LocationProvider* _location;
  bool gps_detected = false;
  bool gps_active = false;
  void initBasicGPS();  // Actual production implementation included below.
  void start_gps() { gps_active = true; _location->begin(); _location->reset(); }
  void stop_gps() { gps_active = false; _location->stop(); }
public:
  int loop_calls = 0, delegated_calls = 0;
  std::string delegated_name, delegated_value;
  explicit EnvironmentSensorManager(LocationProvider& location) : _location(&location) {}
  virtual ~EnvironmentSensorManager() = default;
  virtual bool begin() { initBasicGPS(); return true; }
  virtual void loop() { loop_calls++; }
  virtual bool setSettingValue(const char* name, const char* value) {
    delegated_calls++;
    delegated_name = name;
    delegated_value = value;
    return delegated_name == "gps_interval";
  }
  bool active() const { return gps_active; }
  bool detected() const { return gps_detected; }
};

#include "gps-production.inc"

static void resetHardware(int mode) {
  now_ms = 0;
  switch_level = mode;
  input_configurations = delayed_ms = power_off_calls = 0;
  Serial1 = {};
  pwr_btn = {};
}

static void assertUartInitialized() {
  assert(Serial1.pin_calls == 1 && Serial1.begin_calls == 1);
  assert(Serial1.baud == 9600);
  assert(Serial1.availability_calls == 0 && delayed_ms == 0);
  assert(input_configurations == 1 && pwr_btn.begin_calls == 1);
}

#ifdef MUZI_BASE_SUPERIO
static void physicalSwitchStartupAndRejectedWrites() {
  for (int mode : {LOW, HIGH}) {
    resetHardware(mode);
    LocationProvider location;
    MuziBaseSensorManager manager(location);
    assert(manager.begin());
    assertUartInitialized();
    assert(manager.detected());
    assert(manager.active() == (mode == HIGH));
    // Mode 1 never probes the receiver; Mode 2 performs only its real startup.
    assert(location.begin_calls == (mode == HIGH ? 1 : 0));
    assert(location.reset_calls == location.begin_calls);
    assert(location.stop_calls == 0);
    assert(location.enabled == manager.active());

    for (const char* requested : {"0", "1", "invalid"}) {
      assert(!manager.setSettingValue("gps", requested));
      assert(manager.active() == (mode == HIGH));
      assert(location.begin_calls == (mode == HIGH ? 1 : 0));
      assert(location.stop_calls == 0);
      assert(manager.delegated_calls == 0);
    }

    assert(manager.setSettingValue("gps_interval", "17"));
    assert(manager.delegated_calls == 1);
    assert(manager.delegated_name == "gps_interval" && manager.delegated_value == "17");
    assert(!manager.setSettingValue("not_a_setting", "1"));
    assert(manager.delegated_calls == 2);
    assert(manager.active() == (mode == HIGH));
    assert(location.stop_calls == 0);
  }
}

static void pollingTransitionsAndRollover() {
  resetHardware(LOW);
  LocationProvider location;
  MuziBaseSensorManager manager(location);
  assert(manager.begin());
  switch_level = HIGH;
  now_ms = 199;
  manager.loop();
  assert(!manager.active() && location.begin_calls == 0);
  now_ms = 200;
  manager.loop();
  assert(manager.active() && location.begin_calls == 1 && location.reset_calls == 1);
  for (now_ms = 400; now_ms <= 1000; now_ms += 200) manager.loop();
  assert(location.begin_calls == 1 && location.stop_calls == 0);
  assert(!manager.setSettingValue("gps", "0"));
  assert(manager.active() && location.stop_calls == 0);

  switch_level = LOW;
  now_ms = 1199;
  manager.loop();
  assert(manager.active());
  now_ms = 1200;
  manager.loop();
  assert(!manager.active() && location.stop_calls == 1);
  for (now_ms = 1400; now_ms <= 2000; now_ms += 200) manager.loop();
  assert(location.begin_calls == 1 && location.stop_calls == 1);
  assert(!manager.setSettingValue("gps", "1"));
  assert(!manager.active() && location.begin_calls == 1);

  // The polling subtraction must remain bounded across millis() wraparound.
  now_ms = 0xFFFFFFF0U;
  manager.loop();
  switch_level = HIGH;
  now_ms = 183U;  // 199 ms later, modulo 2^32.
  manager.loop();
  assert(!manager.active());
  now_ms = 184U;
  manager.loop();
  assert(manager.active() && location.begin_calls == 2 && location.reset_calls == 2);
  now_ms = 384U;
  manager.loop();
  assert(location.begin_calls == 2 && location.stop_calls == 1);
  assert(manager.loop_calls > 10 && power_off_calls == 0);
}
#else
static void legacySoftwareGateIsPreserved() {
  for (int mode : {LOW, HIGH}) {
    resetHardware(mode);
    LocationProvider location;
    MuziBaseSensorManager manager(location);
    assert(manager.begin());
    assertUartInitialized();
    // Legacy ENV_SKIP_GPS_DETECT still performs its original probe and stop.
    assert(manager.detected() && !manager.active());
    assert(location.begin_calls == 1 && location.reset_calls == 1 && location.stop_calls == 1);
    assert(manager.setSettingValue("gps", "1"));
    assert(manager.active() == (mode == HIGH));
    assert(manager.delegated_calls == 0);
    assert(manager.setSettingValue("gps", "0"));
    assert(!manager.active());
    assert(manager.setSettingValue("gps_interval", "17"));
    assert(manager.delegated_calls == 1 && manager.delegated_name == "gps_interval");
  }

  resetHardware(HIGH);
  LocationProvider location;
  MuziBaseSensorManager manager(location);
  assert(manager.begin());
  assert(manager.setSettingValue("gps", "1") && manager.active());
  switch_level = LOW;
  now_ms = 200;
  manager.loop();
  assert(!manager.active());
  switch_level = HIGH;
  now_ms = 400;
  manager.loop();
  assert(manager.active());
  assert(manager.setSettingValue("gps", "0") && !manager.active());
  switch_level = LOW;
  now_ms = 600;
  manager.loop();
  switch_level = HIGH;
  now_ms = 800;
  manager.loop();
  assert(!manager.active());  // Hardware alone must not bypass legacy software Off.
}
#endif

int main() {
#ifdef MUZI_BASE_SUPERIO
  physicalSwitchStartupAndRejectedWrites();
  pollingTransitionsAndRollover();
  std::cout << "SuperBase physical-only GPS switch: PASS\n";
#else
  legacySoftwareGateIsPreserved();
  std::cout << "Legacy GPS software gate and probe: PASS\n";
#endif
}
