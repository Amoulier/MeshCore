#pragma once

// Hardware boundary for native CommonRadioPrefs command tests.
struct MockRadioPrefsDriver {
  bool setRxBoostedGainMode(bool) { return true; }
  void setTxPower(int8_t) {}
};
inline MockRadioPrefsDriver radio_driver;

template <typename T> T constrain(T value, T low, T high) {
  return value < low ? low : (value > high ? high : value);
}
