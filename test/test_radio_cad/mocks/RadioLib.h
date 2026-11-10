#pragma once
#include <Arduino.h>
#include <algorithm>
#include <cstdint>
using std::max;
using std::min;

#define LORA_SF 7
#define RADIOLIB_ERR_NONE 0
#define RADIOLIB_CHANNEL_FREE 1
#define RADIOLIB_LORA_DETECTED 2
#define HIGH 1
inline int digitalRead(uint32_t) { return 0; }
inline long random(long minimum, long) { return minimum; }

class PhysicalLayer {
public:
  int16_t scan_result = RADIOLIB_CHANNEL_FREE;
  int scans = 0, starts = 0, output_power_writes = 0, tx_starts = 0;
  void setPacketReceivedAction(void (*)()) {}
  void setPreambleLength(uint16_t) {}
  long random(long) { return 1; }
  uint8_t randomByte() { return 0; }
  int16_t setOutputPower(int8_t) { output_power_writes++; return 0; }
  int16_t standby() { return 0; }
  int16_t sleep() { return 0; }
  int16_t startReceive() { starts++; return 0; }
  int16_t startTransmit(uint8_t*, int) { tx_starts++; return 0; }
  int16_t finishTransmit() { return 0; }
  int16_t scanChannel() { scans++; return scan_result; }
  int getPacketLength() { return 0; }
  int16_t readData(uint8_t*, int) { return 0; }
  uint32_t getTimeOnAir(int) { return 100000; }
  float getRSSI() { return -100; }
  float getSNR() { return 8; }
};
