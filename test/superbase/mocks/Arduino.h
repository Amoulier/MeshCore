#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3
#define CHANGE 4
inline uint32_t mock_now = 0;
inline int mock_pins[64] = {};
inline void (*mock_irqs[64])() = {};
inline uint32_t millis() { return mock_now; }
inline void (*mock_delay_hook)() = nullptr;
inline void delay(uint32_t ms) { mock_now += ms; if (mock_delay_hook) mock_delay_hook(); }
inline long random(long low, long) { return low; }
inline void pinMode(int, int) {}
inline int digitalRead(int pin) { return mock_pins[pin]; }
inline int analogRead(int pin) { return mock_pins[pin]; }
inline void digitalWrite(int pin, int value) { mock_pins[pin] = value; }
inline int attachInterrupt(int pin, void (*fn)(), int) { mock_irqs[pin] = fn; return 1; }
using std::min;
using std::max;
