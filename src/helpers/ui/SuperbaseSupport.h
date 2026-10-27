#pragma once

#include <stdint.h>

namespace superbase {

enum class ChargeState : uint8_t { Battery, Idle, Charging, RecoverableFault, LatchedFault };

inline ChargeState chargeState(bool powered, bool stat1, bool stat2) {
  if (!powered) return ChargeState::Battery;
  if (!stat1) return stat2 ? ChargeState::RecoverableFault : ChargeState::LatchedFault;
  return stat2 ? ChargeState::Idle : ChargeState::Charging;
}

inline bool chargeFault(ChargeState state) {
  return state == ChargeState::RecoverableFault || state == ChargeState::LatchedFault;
}

inline uint8_t batteryPercent(int millivolts) {
  if (millivolts <= 3000) return 0;
  if (millivolts >= 4200) return 100;
  return (millivolts - 3000) * 100 / 1200;
}

enum BuzzerMode : uint8_t { All, Silent, Notifications, System, DirectOnly, ModeCount };

inline bool allowSound(uint8_t mode, bool message, bool direct = false) {
  if (mode == All) return true;
  if (mode == Notifications) return message;
  if (mode == System) return !message;
  return mode == DirectOnly && message && direct;
}

inline const char* soundLabel(uint8_t mode) {
  static const char* names[] = {"All", "Off", "Messages", "System", "DMs Only"};
  return names[mode < ModeCount ? mode : uint8_t(Silent)];
}

// Keep delivery pending until the service write and subscribed notification succeed.
class BatteryDelivery {
  int _stored = -1, _notified = -1;
public:
  void disconnected() { _notified = -1; }
  template<class Service>
  void update(Service& service, uint8_t level, bool connected, uint16_t handle) {
    if (level > 100) level = 100;
    if (_stored != level) {
      if (!service.write(level)) return;
      _stored = level;
    }
    if (!connected) { disconnected(); return; }
    if (_notified != level && service.notify(handle, level)) _notified = level;
  }
};

// A bounded retry interval also works at millis()==0 and through clock rollover.
class RetryTimer {
  uint32_t _last = 0;
  bool _attempted = false;
public:
  bool ready(uint32_t now, uint32_t interval) const {
    return !_attempted || uint32_t(now - _last) >= interval;
  }
  void attempted(uint32_t now) { _last = now; _attempted = true; }
  void clear() { _attempted = false; }
};

// Commit by rename only after a complete write; an interrupted temp file is expendable.
template<class FS, class Prefs>
bool savePreferences(FS& fs, Prefs& prefs, uint8_t writeMode) {
  const char* temporary = "/prefs.tmp";
  if (fs.exists(temporary) && !fs.remove(temporary)) return false;
  auto file = fs.open(temporary, writeMode);
  if (!file) return false;
  bool complete = prefs.saveSerial(file);
  file.flush();
  file.close();
  return complete && fs.rename(temporary, "/prefs.json");
}

} // namespace superbase
