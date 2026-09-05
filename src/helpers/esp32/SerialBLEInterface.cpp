#include "SerialBLEInterface.h"
#include "esp_mac.h"
#include <esp_gap_ble_api.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ADVERT_RESTART_DELAY 1000UL
#define BLE_FAST_ADV_DURATION_MS 60000UL
#define BLE_IDLE_CONNECTION_AFTER_MS 10000UL

// Advertising units are 0.625 ms.
#define BLE_FAST_ADV_MIN 160
#define BLE_FAST_ADV_MAX 240
#define BLE_SLOW_ADV_MIN 800
#define BLE_SLOW_ADV_MAX 1600

void SerialBLEInterface::begin(const char *prefix, char *name, uint32_t pin_code)
{
  _pin_code = pin_code;

  if (strcmp(name, "@@MAC") == 0) {
    uint8_t addr[8] = {};
    esp_efuse_mac_get_default(addr);
    sprintf(name, "%02X%02X%02X%02X%02X%02X",
            addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
  }
  char dev_name[48];
  snprintf(dev_name, sizeof(dev_name), "%s%s", prefix, name);

  BLEDevice::init(dev_name);
  BLEDevice::setSecurityCallbacks(this);
  BLEDevice::setMTU(MAX_FRAME_SIZE);

  BLESecurity sec;
  sec.setStaticPIN(pin_code);
  sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);
  pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic =
      pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  pRxCharacteristic->setCallbacks(this);

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
}

void SerialBLEInterface::startAdvertising(bool fast)
{
  if (!_isEnabled || pServer == NULL) return;
  BLEAdvertising *advertising = pServer->getAdvertising();
  advertising->stop();
  advertising->setMinInterval(fast ? BLE_FAST_ADV_MIN : BLE_SLOW_ADV_MIN);
  advertising->setMaxInterval(fast ? BLE_FAST_ADV_MAX : BLE_SLOW_ADV_MAX);
  advertising->start();
  advertising_fast = fast;
  fast_adv_until = fast ? millis() + BLE_FAST_ADV_DURATION_MS : 0;
  adv_restart_time = 0;
}

void SerialBLEInterface::markActivity()
{
  last_activity_time = millis();
  if (deviceConnected && idle_connection_profile) {
    requestConnectionProfile(false);
  }
}

void SerialBLEInterface::requestConnectionProfile(bool idle)
{
#if defined(HELTEC_V4_BLE_POWER_SAVE) && HELTEC_V4_BLE_POWER_SAVE
  if (!deviceConnected || !peer_address_valid) return;
  if (connection_profile_known && idle_connection_profile == idle) return;

  esp_ble_conn_update_params_t params = {};
  memcpy(params.bda, peer_address, ESP_BD_ADDR_LEN);
  if (idle) {
    params.min_int = 48;
    params.max_int = 96;
    params.latency = 2;
    params.timeout = 500;
  } else {
    params.min_int = 12;
    params.max_int = 24;
    params.latency = 0;
    params.timeout = 400;
  }
  if (esp_ble_gap_update_conn_params(&params) == ESP_OK) {
    idle_connection_profile = idle;
    connection_profile_known = true;
  }
#else
  (void)idle;
#endif
}

void SerialBLEInterface::updatePowerPolicy()
{
  const unsigned long now = millis();

#if defined(HELTEC_V4_BLE_AUTO_OFF_MSEC) && HELTEC_V4_BLE_AUTO_OFF_MSEC > 0
  // The explicit low-power Companion trades unattended discoverability for
  // lower draw. Bluetooth can be restored locally from the node's Bluetooth
  // page with a long PRG press; the standard BLE target never auto-disables.
  if (_isEnabled && pServer->getConnectedCount() == 0 && disconnected_since &&
      static_cast<uint32_t>(now - disconnected_since) >= HELTEC_V4_BLE_AUTO_OFF_MSEC) {
    disable();
    return;
  }
#endif

  if (!deviceConnected && advertising_fast && fast_adv_until &&
      static_cast<long>(now - fast_adv_until) >= 0) {
    startAdvertising(false);
  }
  if (deviceConnected && last_activity_time &&
      static_cast<uint32_t>(now - last_activity_time) >= BLE_IDLE_CONNECTION_AFTER_MS) {
    requestConnectionProfile(true);
  }
}

uint32_t SerialBLEInterface::onPassKeyRequest()
{
  BLE_DEBUG_PRINTLN("onPassKeyRequest()");
  return _pin_code;
}

void SerialBLEInterface::onPassKeyNotify(uint32_t pass_key)
{
  BLE_DEBUG_PRINTLN("onPassKeyNotify(%u)", pass_key);
}

bool SerialBLEInterface::onConfirmPIN(uint32_t pass_key)
{
  BLE_DEBUG_PRINTLN("onConfirmPIN(%u)", pass_key);
  return true;
}

bool SerialBLEInterface::onSecurityRequest()
{
  BLE_DEBUG_PRINTLN("onSecurityRequest()");
  return true;
}

void SerialBLEInterface::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl)
{
  if (cmpl.success) {
    BLE_DEBUG_PRINTLN("Authentication Success");
    deviceConnected = true;
    disconnected_since = 0;
    connection_profile_known = false;
    markActivity();
  } else {
    BLE_DEBUG_PRINTLN("Authentication Failure");
    disconnected_since = millis();
    pServer->disconnect(pServer->getConnId());
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  }
}

void SerialBLEInterface::onConnect(BLEServer *pServer)
{
  (void)pServer;
  disconnected_since = 0;
}

void SerialBLEInterface::onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  BLE_DEBUG_PRINTLN("onConnect(), conn_id=%d, mtu=%d",
                    param->connect.conn_id, pServer->getPeerMTU(param->connect.conn_id));
  last_conn_id = param->connect.conn_id;
  memcpy(peer_address, param->connect.remote_bda, ESP_BD_ADDR_LEN);
  peer_address_valid = true;
  disconnected_since = 0;
  connection_profile_known = false;
  last_activity_time = millis();
}

void SerialBLEInterface::onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  BLE_DEBUG_PRINTLN("onMtuChanged(), mtu=%d", pServer->getPeerMTU(param->mtu.conn_id));
  markActivity();
}

void SerialBLEInterface::onDisconnect(BLEServer *pServer)
{
  (void)pServer;
  BLE_DEBUG_PRINTLN("onDisconnect()");
  deviceConnected = false;
  peer_address_valid = false;
  connection_profile_known = false;
  disconnected_since = millis();
  if (_isEnabled) adv_restart_time = millis() + ADVERT_RESTART_DELAY;
}

void SerialBLEInterface::onWrite(BLECharacteristic *pCharacteristic,
                                 esp_ble_gatts_cb_param_t *param)
{
  (void)param;
  uint8_t *rxValue = pCharacteristic->getData();
  int len = pCharacteristic->getLength();

  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), frame too big, len=%d", len);
  } else {
    Frame frame = {};
    frame.len = len;
    memcpy(frame.buf, rxValue, len);
    if (xQueueSend(recv_queue, &frame, 0) != pdTRUE) {
      BLE_DEBUG_PRINTLN("ERROR: onWrite(), recv_queue is full!");
    } else {
      markActivity();
    }
  }
}

void SerialBLEInterface::clearBuffers()
{
  xQueueReset(recv_queue);
  send_queue_len = 0;
}

void SerialBLEInterface::enable()
{
  if (_isEnabled) return;
  _isEnabled = true;
  disconnected_since = millis();
  clearBuffers();
  pService->start();
  startAdvertising(true);
}

void SerialBLEInterface::disable()
{
  _isEnabled = false;
  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");
  pServer->getAdvertising()->stop();
  if (pServer->getConnectedCount() > 0) pServer->disconnect(last_conn_id);
  pService->stop();
  oldDeviceConnected = deviceConnected = false;
  peer_address_valid = false;
  connection_profile_known = false;
  disconnected_since = 0;
  adv_restart_time = fast_adv_until = 0;
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len)
{
  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("writeFrame(), frame too big, len=%d", len);
    return 0;
  }
  if (deviceConnected && len > 0) {
    if (send_queue_len >= FRAME_QUEUE_SIZE) {
      BLE_DEBUG_PRINTLN("writeFrame(), send_queue is full!");
      return 0;
    }
    send_queue[send_queue_len].len = len;
    memcpy(send_queue[send_queue_len].buf, src, len);
    send_queue_len++;
    markActivity();
    return len;
  }
  return 0;
}

#define BLE_WRITE_MIN_INTERVAL 60

bool SerialBLEInterface::isWriteBusy() const
{
  return millis() < _last_write + BLE_WRITE_MIN_INTERVAL;
}

size_t SerialBLEInterface::checkRecvFrame(uint8_t dest[])
{
  if (send_queue_len > 0 && millis() >= _last_write + BLE_WRITE_MIN_INTERVAL) {
    _last_write = millis();
    pTxCharacteristic->setValue(send_queue[0].buf, send_queue[0].len);
    pTxCharacteristic->notify();
    markActivity();

    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) send_queue[i] = send_queue[i + 1];
  }

  Frame frame;
  if (xQueueReceive(recv_queue, &frame, 0) == pdTRUE) {
    memcpy(dest, frame.buf, frame.len);
    markActivity();
    return frame.len;
  }

  if (deviceConnected != oldDeviceConnected) {
    if (!deviceConnected) {
      clearBuffers();
      disconnected_since = millis();
      adv_restart_time = millis() + ADVERT_RESTART_DELAY;
    } else {
      pServer->getAdvertising()->stop();
      disconnected_since = 0;
      adv_restart_time = fast_adv_until = 0;
      requestConnectionProfile(false);
    }
    oldDeviceConnected = deviceConnected;
  }

  if (adv_restart_time && static_cast<long>(millis() - adv_restart_time) >= 0) {
    if (pServer->getConnectedCount() == 0) startAdvertising(true);
    adv_restart_time = 0;
  }

  updatePowerPolicy();
  return 0;
}

bool SerialBLEInterface::isConnected() const
{
  return deviceConnected;
}
