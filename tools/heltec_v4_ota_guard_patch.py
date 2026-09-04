from pathlib import Path

header = Path("variants/heltec_v4/HeltecV4Board.h")
text = header.read_text(encoding="utf-8")
old = "  bool handleCommand(const char *command, uint32_t sender_timestamp, char *reply) override;\n\n  void onBeforeTransmit() override;"
new = "  bool handleCommand(const char *command, uint32_t sender_timestamp, char *reply) override;\n  bool startOTAUpdate(const char *id, char reply[]) override;\n\n  void onBeforeTransmit() override;"
if text.count(old) != 1:
    raise RuntimeError("HeltecV4Board.h insertion point did not match")
header.write_text(text.replace(old, new), encoding="utf-8")

source = Path("variants/heltec_v4/HeltecV4Board.cpp")
text = source.read_text(encoding="utf-8")
old_loop = '''void HeltecV4Board::loop()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  const uint32_t now = millis();
'''
new_loop = '''void HeltecV4Board::loop()
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  if (inhibit_sleep) {
    return;
  }

  const uint32_t now = millis();
'''
if text.count(old_loop) != 1:
    raise RuntimeError("HeltecV4Board::loop insertion point did not match")
text = text.replace(old_loop, new_loop)

old_poweroff = '''void HeltecV4Board::powerOff()
{
'''
new_ota_and_poweroff = '''bool HeltecV4Board::startOTAUpdate(const char *id, char reply[])
{
#if defined(HELTEC_V4_SOLAR_PROFILE) && HELTEC_V4_SOLAR_PROFILE
  const uint16_t battery_millivolts = readBatteryMilliVoltsRaw();
  if (battery_millivolts < HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS) {
    sprintf(reply, "Error: battery %umV; OTA requires at least %umV",
            static_cast<unsigned>(battery_millivolts),
            static_cast<unsigned>(HELTEC_V4_BATTERY_RECOVERY_MILLIVOLTS));
    return false;
  }
#endif
  return ESP32Board::startOTAUpdate(id, reply);
}

void HeltecV4Board::powerOff()
{
'''
if text.count(old_poweroff) != 1:
    raise RuntimeError("HeltecV4Board::powerOff insertion point did not match")
source.write_text(text.replace(old_poweroff, new_ota_and_poweroff), encoding="utf-8")

print("Heltec V4 OTA battery and sleep guard applied")
