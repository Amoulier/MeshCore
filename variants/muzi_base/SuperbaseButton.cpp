#ifdef MUZI_BASE_SUPERIO
#include "SuperbaseButton.h"

SuperbaseButton* SuperbaseButton::_buttons[6] = {};
void SuperbaseButton::irq0() { _buttons[0]->capture(); }
void SuperbaseButton::irq1() { _buttons[1]->capture(); }
void SuperbaseButton::irq2() { _buttons[2]->capture(); }
void SuperbaseButton::irq3() { _buttons[3]->capture(); }
void SuperbaseButton::irq4() { _buttons[4]->capture(); }
void SuperbaseButton::irq5() { _buttons[5]->capture(); }

void SuperbaseButton::begin() {
  pinMode(_pin, INPUT_PULLUP);
  _buttons[_slot] = this;
  void (*callbacks[])() = {irq0, irq1, irq2, irq3, irq4, irq5};
  int attached = attachInterrupt(_pin, callbacks[_slot], CHANGE);
#ifdef SUPERBASE_USB_DIAGNOSTICS
  interruptAttached = attached != 0;
#endif
}

void SuperbaseButton::capture() {
#ifdef SUPERBASE_USB_DIAGNOSTICS
  capturedEdges++;
#endif
  uint8_t next = (_write + 1) % 8;
  if (next == _read) {
#ifdef SUPERBASE_USB_DIAGNOSTICS
    droppedEdges++;
#endif
    return;
  }
  _edges[_write].at = millis();
  _edges[_write].down = digitalRead(_pin) == LOW;
  _write = next;
}

int SuperbaseButton::edge(bool down, uint32_t at) {
  _sampled = down;
  if (down) {
    if (!_held && (!_released || uint32_t(at - _released_at) >= 12)) {
      _held = true;
      _long_sent = _cancelled = false;
      _pressed_at = at;
    }
    return BUTTON_EVENT_NONE;
  }
  if (!_held) return BUTTON_EVENT_NONE;
  _held = false;
  uint32_t duration = at - _pressed_at;
  if (_cancelled || _long_sent || duration < 12) return BUTTON_EVENT_NONE;
  _released = true;
  _released_at = at;
  return _long_ms && duration >= _long_ms ? BUTTON_EVENT_LONG_PRESS : BUTTON_EVENT_CLICK;
}

int SuperbaseButton::click(int event, uint32_t now) {
  if (event == BUTTON_EVENT_LONG_PRESS) { _clicks = 0; return event; }
  if (event == BUTTON_EVENT_CLICK && _multi) {
    _clicks++;
    _click_at = now;
    if (_clicks == 3) { _clicks = 0; return BUTTON_EVENT_TRIPLE_CLICK; }
    return BUTTON_EVENT_NONE;
  }
  return event;
}

int SuperbaseButton::check(bool repeat) {
  uint32_t now = millis();
  while (_read != _write) {
    uint8_t index = _read;
    uint32_t at = _edges[index].at;
    if (_clicks && !_held && uint32_t(at - _click_at) >= 280) {
      int pending = _clicks == 1 ? BUTTON_EVENT_CLICK : BUTTON_EVENT_DOUBLE_CLICK;
      _clicks = 0;
      return pending;
    }
    bool down = _edges[index].down;
    _read = (index + 1) % 8;
    int event = click(edge(down, at), at);
    if (event) return event;
  }
  bool down = digitalRead(_pin) == LOW;
  if (down != _sampled) {
    int event = click(edge(down, now), now);
    if (event) return event;
  }
  if (_held && !_cancelled && _long_ms && uint32_t(now - _pressed_at) >= _long_ms && (!_long_sent || repeat)) {
    _long_sent = true;
    return click(BUTTON_EVENT_LONG_PRESS, now);
  }
  if (_clicks && !_held && uint32_t(now - _click_at) >= 280) {
    int event = _clicks == 1 ? BUTTON_EVENT_CLICK : BUTTON_EVENT_DOUBLE_CLICK;
    _clicks = 0;
    return event;
  }
  return BUTTON_EVENT_NONE;
}
#endif
