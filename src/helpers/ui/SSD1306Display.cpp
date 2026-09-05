#include "SSD1306Display.h"

#if defined(HELTEC_V4_PERSISTENT_DISPLAY) && HELTEC_V4_PERSISTENT_DISPLAY
#include <Preferences.h>
#include <helpers/PersistentWriteGuard.h>
#endif

namespace {
constexpr const char *DISPLAY_PREFS_NAMESPACE = "meshcore_v4";
constexpr const char *DISPLAY_DISABLED_KEY = "display_off";

}

bool SSD1306Display::i2c_probe(TwoWire &wire, uint8_t addr)
{
  wire.beginTransmission(addr);
  return wire.endTransmission() == 0;
}

ColorVal UIColor::window_bkg = SSD1306_BLACK;
ColorVal UIColor::title_bkg = SSD1306_BLACK;
ColorVal UIColor::title_txt = SSD1306_WHITE;
ColorVal UIColor::primary_txt = SSD1306_WHITE;
ColorVal UIColor::secondary_txt = SSD1306_WHITE;
ColorVal UIColor::warning_txt = SSD1306_WHITE;
ColorVal UIColor::popup_bkg = SSD1306_BLACK;
ColorVal UIColor::popup_txt = SSD1306_WHITE;
ColorVal UIColor::corp_blue = SSD1306_WHITE;

bool SSD1306Display::loadPersistentDisabled()
{
#if defined(HELTEC_V4_PERSISTENT_DISPLAY) && HELTEC_V4_PERSISTENT_DISPLAY
  Preferences preferences;
  if (!preferences.begin(DISPLAY_PREFS_NAMESPACE, true)) {
    return HELTEC_V4_DISPLAY_DEFAULT_OFF != 0;
  }
  const bool disabled = preferences.getBool(DISPLAY_DISABLED_KEY, HELTEC_V4_DISPLAY_DEFAULT_OFF != 0);
  preferences.end();
  return disabled;
#else
  return false;
#endif
}

void SSD1306Display::savePersistentDisabled(bool disabled)
{
#if defined(HELTEC_V4_PERSISTENT_DISPLAY) && HELTEC_V4_PERSISTENT_DISPLAY
  if (!meshcorePersistentWritesAllowed()) {
    return;
  }
  Preferences preferences;
  if (preferences.begin(DISPLAY_PREFS_NAMESPACE, false)) {
    preferences.putBool(DISPLAY_DISABLED_KEY, disabled);
    preferences.end();
  }
#else
  (void)disabled;
#endif
}

bool SSD1306Display::initializePanel()
{
  if (_initialized) {
    return true;
  }

  if (_peripher_power && !_railClaimed) {
    _peripher_power->claim();
    _railClaimed = true;
  }
  _isOn = true;
  delay(10);

  const bool started = display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false);
  if (!started || !i2c_probe(Wire, DISPLAY_ADDRESS)) {
    if (_peripher_power && _railClaimed) {
      _peripher_power->release();
      _railClaimed = false;
    }
    _isOn = false;
    _initialized = false;
    return false;
  }

#ifdef DISPLAY_ROTATION
  display.setRotation(DISPLAY_ROTATION);
#endif
  _initialized = true;
  display.ssd1306_command(SSD1306_DISPLAYON);
  return true;
}

void SSD1306Display::powerDownPanel()
{
#if PIN_OLED_RESET >= 0
  pinMode(PIN_OLED_RESET, OUTPUT);
  digitalWrite(PIN_OLED_RESET, LOW);
#endif
  if (_peripher_power && _railClaimed) {
    _peripher_power->release();
    _railClaimed = false;
  }
  _isOn = false;
  _initialized = false;
}

bool SSD1306Display::begin()
{
  if (!_persistentPreferenceLoaded) {
    _persistentlyDisabled = loadPersistentDisabled();
    _persistentPreferenceLoaded = true;
  }

  if (_persistentlyDisabled) {
    powerDownPanel();
    return true;
  }

  return initializePanel();
}

bool SSD1306Display::setPersistentlyDisabled(bool disabled)
{
  _persistentPreferenceLoaded = true;
  _persistentlyDisabled = disabled;
  savePersistentDisabled(disabled);

  if (disabled) {
    turnOff();
    return true;
  }

  turnOn();
  return _isOn;
}

void SSD1306Display::turnOn()
{
  if (_persistentlyDisabled) {
    return;
  }

  if (!initializePanel()) {
    return;
  }
  display.ssd1306_command(SSD1306_DISPLAYON);
  _isOn = true;
}

void SSD1306Display::turnOff()
{
  if (_initialized && _isOn) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
  powerDownPanel();
}

void SSD1306Display::clear()
{
  if (!_initialized || !_isOn) {
    return;
  }
  display.clearDisplay();
  display.display();
}

void SSD1306Display::startFrame(ColorVal bkg)
{
  (void)bkg;
  if (!_initialized || !_isOn) {
    return;
  }
  display.clearDisplay();
  _color = SSD1306_WHITE;
  display.setTextColor(_color);
  display.setTextSize(1);
  display.cp437(true);
}

void SSD1306Display::setTextSize(int sz)
{
  if (_initialized && _isOn) {
    display.setTextSize(sz);
  }
}

void SSD1306Display::setColor(ColorVal c)
{
  _color = c;
  if (_initialized && _isOn) {
    display.setTextColor(_color);
  }
}

void SSD1306Display::setCursor(int x, int y)
{
  if (_initialized && _isOn) {
    display.setCursor(x, y);
  }
}

void SSD1306Display::print(const char *str)
{
  if (_initialized && _isOn) {
    display.print(str);
  }
}

void SSD1306Display::fillRect(int x, int y, int w, int h)
{
  if (_initialized && _isOn) {
    display.fillRect(x, y, w, h, _color);
  }
}

void SSD1306Display::drawRect(int x, int y, int w, int h)
{
  if (_initialized && _isOn) {
    display.drawRect(x, y, w, h, _color);
  }
}

void SSD1306Display::drawXbm(int x, int y, const uint8_t *bits, int w, int h)
{
  if (_initialized && _isOn) {
    display.drawBitmap(x, y, bits, w, h, _color);
  }
}

uint16_t SSD1306Display::getTextWidth(const char *str)
{
  if (!_initialized || !_isOn) {
    return 0;
  }
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
  return w;
}

void SSD1306Display::endFrame()
{
  if (_initialized && _isOn) {
    display.display();
  }
}
