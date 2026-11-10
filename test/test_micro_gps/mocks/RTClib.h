#pragma once
#include <stdint.h>

// Calendar boundary fake, NOT a replacement test of Adafruit RTClib. It models
// DateTime(year,month,day,hour,minute,second), isValid(), and uint32 unixtime().
// RTClib's public calendar contract supports years 2000 through 2099.
class DateTime {
  uint16_t _year;
  uint8_t _month, _day, _hour, _minute, _second;
  static bool leap(uint16_t year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
  static uint8_t monthDays(uint16_t year, uint8_t month) {
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && leap(year) ? 29 : days[month - 1];
  }
public:
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
      : _year(year), _month(month), _day(day), _hour(hour), _minute(minute), _second(second) {}
  bool isValid() const {
    return _year >= 2000 && _year <= 2099 && _month >= 1 && _month <= 12 &&
           _day >= 1 && _day <= monthDays(_year, _month) && _hour <= 23 && _minute <= 59 && _second <= 59;
  }
  uint32_t unixtime() const {
    uint32_t days = 0;
    for (uint16_t year = 1970; year < _year; ++year) days += leap(year) ? 366 : 365;
    for (uint8_t month = 1; month < _month; ++month) days += monthDays(_year, month);
    days += _day - 1;
    return days * 86400U + _hour * 3600U + _minute * 60U + _second;
  }
};
