#pragma once

#include "../BaseSerialInterface.h"
#include <WiFi.h>

class SerialWifiInterface : public BaseSerialInterface {
  bool deviceConnected;
  bool _isEnabled;
  unsigned long _last_write;
  unsigned long adv_restart_time;

  WiFiServer server;
  mutable WiFiClient client; // available() is logically a query but not const in Arduino.
  int _last_read_available = 0;
  uint32_t _last_read_millis = 0;
  bool expirePartialRead();

  struct FrameHeader {
    uint8_t type;
    uint16_t length;
  };

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  FrameHeader received_frame_header;

  #define FRAME_QUEUE_SIZE  4
  int recv_queue_len;
  Frame recv_queue[FRAME_QUEUE_SIZE];
  int send_queue_len;
  Frame send_queue[FRAME_QUEUE_SIZE];

  void clearBuffers() { recv_queue_len = 0; send_queue_len = 0; }

protected:

public:
  SerialWifiInterface() : server(WiFiServer()), client(WiFiClient()) {
    deviceConnected = false;
    _isEnabled = false;
    _last_write = 0;
    send_queue_len = recv_queue_len = 0;
    received_frame_header.type = 0;
    received_frame_header.length = 0;
  }

  void begin(int port);

  // BaseSerialInterface methods
  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }

  bool isConnected() const override;
  bool isReadBusy() const override;
  bool isWriteBusy() const override;

  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;

  bool hasReceivedFrameHeader() const;
  void resetReceivedFrameHeader();
};

#if WIFI_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define WIFI_DEBUG_PRINT(F, ...) Serial.printf("WiFi: " F, ##__VA_ARGS__)
  #define WIFI_DEBUG_PRINTLN(F, ...) Serial.printf("WiFi: " F "\n", ##__VA_ARGS__)
#else
  #define WIFI_DEBUG_PRINT(...) {}
  #define WIFI_DEBUG_PRINTLN(...) {}
#endif
