#pragma once

#include <helpers/ui/MomentaryButton.h>

// Capture both edges so a tap during cooperative sleep survives until the UI polls.
class SuperbaseButton : public MomentaryButton {
  struct Edge { uint32_t at; bool down; };
  volatile Edge _edges[8];
  volatile uint8_t _read = 0, _write = 0;
  uint8_t _slot;
  int8_t _pin;
  bool _sampled = false, _held = false, _long_sent = false, _cancelled = false;
  uint32_t _pressed_at = 0, _released_at = 0, _click_at = 0;
  bool _released = false;
  uint8_t _clicks = 0;
  bool _multi;
  uint32_t _long_ms;
  static SuperbaseButton* _buttons[6];
  static void irq0(); static void irq1(); static void irq2();
  static void irq3(); static void irq4(); static void irq5();
  int edge(bool down, uint32_t at);
  int click(int event, uint32_t now);
public:
  SuperbaseButton(uint8_t slot, int8_t pin, uint32_t long_ms = 1000, bool multi = false)
    : MomentaryButton(pin, long_ms, true, true, false), _slot(slot), _pin(pin), _multi(multi), _long_ms(long_ms) {}
  void begin() override;
  int check(bool repeat = false) override;
  void capture();
  void cancelClick() { _cancelled = true; _clicks = 0; }
};
