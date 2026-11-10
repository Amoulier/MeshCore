#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

inline uint32_t g_mock_millis = 0;
inline uint32_t millis() { return g_mock_millis; }
inline void delay(uint32_t ms) { g_mock_millis += ms; }
#ifndef PIN_GPS_RX
#define PIN_GPS_RX 38
#endif
#ifndef PIN_GPS_TX
#define PIN_GPS_TX 39
#endif
#ifndef PIN_GPS_EN
#define PIN_GPS_EN 34
#endif

class MockGpsSerial {
public:
  int begin_calls = 0, end_calls = 0, available_calls = 0, set_pins_calls = 0;
  bool running = false;
  void setPins(int, int) { set_pins_calls++; }
  void begin(unsigned long) { begin_calls++; running = true; }
  void end() { end_calls++; running = false; }
  int available() { available_calls++; return 0; }
};
inline MockGpsSerial Serial1;
