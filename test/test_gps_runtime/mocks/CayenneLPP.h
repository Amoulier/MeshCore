#pragma once
#include <cstddef>
#include <cstdint>
class CayenneLPP {
public:
  int gps_calls = 0;
  double latitude = 0, longitude = 0, altitude = 0;
  explicit CayenneLPP(size_t = 128) {}
  void addGPS(uint8_t, double lat, double lon, double alt) {
    gps_calls++; latitude = lat; longitude = lon; altitude = alt;
  }
};
