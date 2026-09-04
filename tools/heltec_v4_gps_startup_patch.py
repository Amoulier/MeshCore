from pathlib import Path

path = Path("src/helpers/sensors/EnvironmentSensorManager.cpp")
text = path.read_text(encoding="utf-8")
old = '''  #ifdef GPS_BAUD_RATE
  Serial1.begin(GPS_BAUD_RATE);
  #else
  Serial1.begin(9600);
  #endif

  // Try to detect if GPS is physically connected to determine if we should expose the setting
  _location->begin();
'''
new = '''  #ifdef GPS_BAUD_RATE
  Serial1.begin(GPS_BAUD_RATE);
  #else
  Serial1.begin(9600);
  #endif

#if defined(HELTEC_V4_SKIP_GPS_STARTUP_PROBE) && HELTEC_V4_SKIP_GPS_STARTUP_PROBE
  // Keep UART ready for a later user enable, but do not power, reset, probe or
  // wait for the optional expansion GPS during boot.
  gps_detected = true;
  _location->stop();
  gps_active = false;
  return;
#endif

  // Try to detect if GPS is physically connected to determine if we should expose the setting
  _location->begin();
'''
if text.count(old) != 1:
    raise RuntimeError(f"expected one GPS init block, found {text.count(old)}")
path.write_text(text.replace(old, new), encoding="utf-8")
print("Heltec V4 GPS startup probe and delay removed")
