#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "../../mocks/Stream.h"

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define PIN_GPS_EN_ACTIVE LOW
#define PIN_GPS_RESET_ACTIVE LOW

inline uint32_t g_mock_millis = 0;
inline uint64_t g_mock_tick_ms = 0;
inline uint8_t g_mock_gpio[64] = {};
inline unsigned g_mock_pin_modes = 0;
inline unsigned g_mock_gpio_writes = 0;

inline void setMockTimeMs(uint64_t now) {
  g_mock_tick_ms = now;
  g_mock_millis = static_cast<uint32_t>(now);
}
inline uint32_t millis() { return g_mock_millis; }
inline void delay(uint32_t milliseconds) { setMockTimeMs(g_mock_tick_ms + milliseconds); }
inline void yield() { delay(1); }
inline void pinMode(int, int) { ++g_mock_pin_modes; }
inline void digitalWrite(int pin, uint8_t level) { g_mock_gpio[pin] = level; ++g_mock_gpio_writes; }
inline int digitalRead(int pin) { return g_mock_gpio[pin]; }
