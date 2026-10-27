#pragma once
#include <Arduino.h>
#define RADIOLIB_ERR_NONE 0
#define RADIOLIB_CHANNEL_FREE 0
#define LORA_SF 8
class PhysicalLayer {
public:
  int receive_calls = 0, receive_result = 0, sleep_calls = 0, scan_calls = 0;
  void (*irq)() = nullptr;
  void setPacketReceivedAction(void (*fn)()) { irq = fn; }
  int setPreambleLength(int) { return 0; }
  int setOutputPower(int) { return 0; }
  int random(int) { return 1; }
  int randomByte() { return 1; }
  int standby() { return 0; }
  int sleep() { sleep_calls++; return 0; }
  int startReceive() { receive_calls++; return receive_result; }
  int getPacketLength() { return 0; }
  int readData(uint8_t*, int) { return 0; }
  uint32_t getTimeOnAir(int) { return 30000; }
  int startTransmit(uint8_t*, int) { return 0; }
  int finishTransmit() { return 0; }
  int scanChannel() { scan_calls++; return 0; }
  float getRSSI() { return -100; }
  float getSNR() { return 1; }
};
