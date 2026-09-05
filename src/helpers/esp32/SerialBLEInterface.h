#pragma once

#include "../BaseSerialInterface.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class SerialBLEInterface : public BaseSerialInterface, BLESecurityCallbacks, BLEServerCallbacks, BLECharacteristicCallbacks {
  BLEServer *pServer;
  BLEService *pService;
  BLECharacteristic *pTxCharacteristic;
  bool deviceConnected;
  bool oldDeviceConnected;
  bool _isEnabled;
  uint16_t last_conn_id;
  uint32_t _pin_code;
  unsigned long _last_write;
  unsigned long adv_restart_time;
  unsigned long fast_adv_until;
  unsigned long last_activity_time;
  bool advertising_fast;
  bool peer_address_valid;
  bool connection_profile_known;
  bool idle_connection_profile;
  uint8_t peer_address[ESP_BD_ADDR_LEN];

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define FRAME_QUEUE_SIZE 4
  StaticQueue_t recv_queue_state;
  uint8_t recv_queue_storage[FRAME_QUEUE_SIZE * sizeof(Frame)];
  QueueHandle_t recv_queue;
  int send_queue_len;
  Frame send_queue[FRAME_QUEUE_SIZE];

  void clearBuffers();
  void startAdvertising(bool fast);
  void markActivity();
  void requestConnectionProfile(bool idle);
  void updatePowerPolicy();

protected:
  uint32_t onPassKeyRequest() override;
  void onPassKeyNotify(uint32_t pass_key) override;
  bool onConfirmPIN(uint32_t pass_key) override;
  bool onSecurityRequest() override;
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;

  void onConnect(BLEServer *pServer) override;
  void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override;
  void onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override;
  void onDisconnect(BLEServer *pServer) override;

  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;

public:
  SerialBLEInterface()
  {
    pServer = NULL;
    pService = NULL;
    pTxCharacteristic = NULL;
    deviceConnected = false;
    oldDeviceConnected = false;
    adv_restart_time = 0;
    fast_adv_until = 0;
    last_activity_time = 0;
    advertising_fast = false;
    peer_address_valid = false;
    connection_profile_known = false;
    idle_connection_profile = false;
    _isEnabled = false;
    _last_write = 0;
    last_conn_id = 0;
    memset(peer_address, 0, sizeof(peer_address));
    recv_queue = xQueueCreateStatic(
        FRAME_QUEUE_SIZE, sizeof(Frame), recv_queue_storage, &recv_queue_state);
    send_queue_len = 0;
  }

  void begin(const char *prefix, char *name, uint32_t pin_code);

  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }
  bool isConnected() const override;
  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;
};

#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINT(F, ...) Serial.printf("BLE: " F, ##__VA_ARGS__)
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINT(...) {}
  #define BLE_DEBUG_PRINTLN(...) {}
#endif
