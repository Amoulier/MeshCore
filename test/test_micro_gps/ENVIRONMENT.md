# Native MicroNMEA provider tests

Run `pio test -e native_micro_gps`. The environment in the root `platformio.ini`
uses MicroNMEA 2.0.6 and the production `MicroNMEALocationProvider` and
`RefCountedDigitalPin` headers. `lib_compat_mode = off` permits the real Arduino
parser to compile against these native peripheral boundaries. This suite is
excluded from the generic native environment because its mocks are isolated.

The fakes cover only UART, GPIO, the monotonic clocks, RTCClock and DateTime's
calendar/uint32-epoch contract. These tests do not test RTClib's implementation
or measure electrical GPS consumption.

The cases cover valid GGA/RMC fixes and CRLF, other or invalid sentences, cached
fix age while powered off, an entire 32-bit millis wrap, shared rail ownership,
cold-probe suppression, GGA without a date, explicit resynchronization, calendar
validity, a date after 2030, epochs after 2038 and the service deadline at rollover.

RTC polling advances in 1001 ms steps to respect the provider's existing positive
deadline comparison. On MinGW/ESP32, the epoch test exercises the signed 32-bit
`long` return path. On hosts with 64-bit `long`, it verifies the unsigned RTC
boundary without claiming to emulate another ABI.
