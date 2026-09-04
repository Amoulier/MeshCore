#include "MomentaryButton.h"

#define MULTI_CLICK_WINDOW_MS 280

__attribute__((noinline)) bool platformHandleLongPress(int8_t pin) __attribute__((weak));
__attribute__((noinline)) bool platformHandleLongPress(int8_t pin)
{
  (void)pin;
  return false;
}

MomentaryButton::MomentaryButton(int8_t pin, int long_press_millis, bool reverse, bool pulldownup, bool multiclick)
{
  _pin = pin;
  _reverse = reverse;
  _pull = pulldownup;
  down_at = 0;
  prev = _reverse ? HIGH : LOW;
  cancel = 0;
  _long_millis = long_press_millis;
  _threshold = 0;
  _click_count = 0;
  _last_click_time = 0;
  _multi_click_window = multiclick ? MULTI_CLICK_WINDOW_MS : 0;
  _pending_click = false;
}

MomentaryButton::MomentaryButton(int8_t pin, int long_press_millis, int analog_threshold)
{
  _pin = pin;
  _reverse = false;
  _pull = false;
  down_at = 0;
  prev = LOW;
  cancel = 0;
  _long_millis = long_press_millis;
  _threshold = analog_threshold;
  _click_count = 0;
  _last_click_time = 0;
  _multi_click_window = MULTI_CLICK_WINDOW_MS;
  _pending_click = false;
}

void MomentaryButton::begin()
{
  if (_pin >= 0 && _threshold == 0) {
    pinMode(_pin, _pull ? (_reverse ? INPUT_PULLUP : INPUT_PULLDOWN) : INPUT);
  }
}

bool MomentaryButton::isPressed() const
{
  const int btn = _threshold > 0 ? (analogRead(_pin) < _threshold) : digitalRead(_pin);
  return isPressed(btn);
}

void MomentaryButton::cancelClick()
{
  cancel = 1;
  down_at = 0;
  _click_count = 0;
  _last_click_time = 0;
  _pending_click = false;
}

bool MomentaryButton::isPressed(int level) const
{
  if (_threshold > 0) {
    return level;
  }
  return _reverse ? level == LOW : level != LOW;
}

int MomentaryButton::check(bool repeat_click)
{
  if (_pin < 0) {
    return BUTTON_EVENT_NONE;
  }

  int event = BUTTON_EVENT_NONE;
  const int btn = _threshold > 0 ? (analogRead(_pin) < _threshold) : digitalRead(_pin);
  if (btn != prev) {
    if (isPressed(btn)) {
      down_at = millis();
    } else {
      if (_long_millis > 0) {
        if (down_at > 0 && static_cast<unsigned long>(millis() - down_at) < static_cast<unsigned long>(_long_millis)) {
          _click_count++;
          _last_click_time = millis();
          _pending_click = true;
        }
      } else {
        _click_count++;
        _last_click_time = millis();
        _pending_click = true;
      }
      if (event == BUTTON_EVENT_CLICK && cancel) {
        event = BUTTON_EVENT_NONE;
        _click_count = 0;
        _last_click_time = 0;
        _pending_click = false;
      }
      down_at = 0;
    }
    prev = btn;
  }

  if (!isPressed(btn) && cancel) {
    cancel = 0;
  }

  if (_long_millis > 0 && down_at > 0 &&
      static_cast<unsigned long>(millis() - down_at) >= static_cast<unsigned long>(_long_millis)) {
    if (_pending_click) {
      cancelClick();
    } else if (platformHandleLongPress(_pin)) {
      cancelClick();
    } else {
      event = BUTTON_EVENT_LONG_PRESS;
      down_at = 0;
      _click_count = 0;
      _last_click_time = 0;
      _pending_click = false;
    }
  }

  if (down_at > 0 && repeat_click) {
    const unsigned long diff = static_cast<unsigned long>(millis() - down_at);
    if (diff >= 700) {
      event = BUTTON_EVENT_CLICK;
    }
  }

  if (_pending_click && (millis() - _last_click_time) >= static_cast<unsigned long>(_multi_click_window)) {
    if (down_at > 0) {
      return event;
    }
    switch (_click_count) {
    case 1:
      event = BUTTON_EVENT_CLICK;
      break;
    case 2:
      event = BUTTON_EVENT_DOUBLE_CLICK;
      break;
    case 3:
    default:
      event = BUTTON_EVENT_TRIPLE_CLICK;
      break;
    }
    _click_count = 0;
    _last_click_time = 0;
    _pending_click = false;
  }

  return event;
}
