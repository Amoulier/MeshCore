#include "SerialBLEInterface.h"
#include "esp_mac.h"
#include <esp_gap_ble_api.h>
#include <esp_bt.h>
#include <esp_bt_main.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ADVERT_RESTART_DELAY 1000UL
#define BLE_FAST_ADV_DURATION_MS 60000UL
#define BLE_IDLE_CONNECTION_AFTER_MS 10000UL

// Advertising units are 0.625 ms.
#define BLE_FAST_ADV_MIN 160
#define BLE_FAST_ADV_MAX 240
#define BLE_SLOW_ADV_MIN 3200
#define BLE_SLOW_ADV_MAX 4800

#if defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) && HELTEC_V4_COMPANION_LIGHT_SLEEP
// Arduino BLE creates attributes by iterating a map keyed by characteristic
// pointer. Separate heap allocations can reverse that order after a rebuild,
// swapping RX/TX handles underneath bonded clients. Members of one allocation
// keep TX before RX regardless of heap reuse, so the attribute order is stable.
struct SerialBLEInterface::BLECharacteristicPair {
  BLECharacteristic tx;
  BLECharacteristic rx;

  BLECharacteristicPair()
    : tx(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY),
      rx(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE) {}
};
#endif

SerialBLEInterface* SerialBLEInterface::power_instance = nullptr;

void SerialBLEInterface::powerGattsEvent(esp_gatts_cb_event_t event, esp_gatt_if_t, esp_ble_gatts_cb_param_t* param) {
  if (!power_instance) return;
  if (event == ESP_GATTS_REG_EVT && param->reg.status == ESP_GATT_OK) power_instance->app_registered = true;
  if (event == ESP_GATTS_DELETE_EVT) power_instance->service_deleted = true;
}

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
  memcpy(saved_device_name, dev_name, sizeof(saved_device_name));

  BLEDevice::init(dev_name);
  BLEDevice::setSecurityCallbacks(this);
  BLEDevice::setMTU(MAX_FRAME_SIZE);

  BLESecurity sec;
  sec.setStaticPIN(pin_code);
  sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);
#if defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) && HELTEC_V4_COMPANION_LIGHT_SLEEP
  power_instance = this;
  BLEDevice::setCustomGattsHandler(powerGattsEvent);
#endif
  createService();
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
}

void SerialBLEInterface::createService()
{
  pService = pServer->createService(SERVICE_UUID);

#if defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) && HELTEC_V4_COMPANION_LIGHT_SLEEP
  pCharacteristicPair = new BLECharacteristicPair();
  pTxCharacteristic = &pCharacteristicPair->tx;
  pRxCharacteristic = &pCharacteristicPair->rx;
  pService->addCharacteristic(pTxCharacteristic);
  pService->addCharacteristic(pRxCharacteristic);
#else
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
#endif
  pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  pNotifyDescriptor = new BLE2902();
  pTxCharacteristic->addDescriptor(pNotifyDescriptor);

#if !defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) || !HELTEC_V4_COMPANION_LIGHT_SLEEP
  pRxCharacteristic =
      pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
#endif
  pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  pRxCharacteristic->setCallbacks(this);

  service_created = false;
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

#if defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) && HELTEC_V4_COMPANION_LIGHT_SLEEP
  if (!_isEnabled) {
    suspendController();
    return;
  }
#endif

#if defined(HELTEC_V4_BLE_AUTO_OFF_MSEC) && HELTEC_V4_BLE_AUTO_OFF_MSEC > 0
  // Companion trades unattended discoverability for lower draw after the
  // configured disconnected interval. Restore Bluetooth locally from the
  // node's Bluetooth page with a long PRG press.
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
  if (!resumeController()) return;
  _isEnabled = true;
  disconnected_since = millis();
  clearBuffers();
  if (!service_created) {
    pService->start();
    service_created = true;
  } else {
    // Reuse the registered attributes rather than adding them on every toggle.
    power_error = esp_ble_gatts_start_service(pService->getHandle());
    if (power_error != ESP_OK) {
      _isEnabled = false;
      return;
    }
  }
  startAdvertising(true);
}

void SerialBLEInterface::disable()
{
  if (!_isEnabled) return;
  _isEnabled = false;
  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");
  pServer->getAdvertising()->stop();
  if (pServer->getConnectedCount() > 0) pServer->disconnect(last_conn_id);
#if !defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) || !HELTEC_V4_COMPANION_LIGHT_SLEEP
  pService->stop();
#endif
  oldDeviceConnected = deviceConnected = false;
  peer_address_valid = false;
  connection_profile_known = false;
  disconnected_since = 0;
  adv_restart_time = fast_adv_until = 0;
  // A live peer disconnects asynchronously; the loop retries after its callback.
  suspendController();
}

void SerialBLEInterface::suspendController()
{
#if defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) && HELTEC_V4_COMPANION_LIGHT_SLEEP
  if (controller_suspended || _isEnabled || pServer->getConnectedCount() != 0) return;
  // Bluedroid discards its GATT database on disable. Unregister and release
  // our wrappers now, so repeated suspend/resume cycles do not leak them.
  if (pService != nullptr) {
    service_deleted = false;
    pServer->removeService(pService);
    const uint32_t deadline = millis() + 2000;
    while (!service_deleted && static_cast<int32_t>(millis() - deadline) < 0) delay(1);
    if (!service_deleted) { power_error = ESP_ERR_TIMEOUT; return; }
    delete pNotifyDescriptor;
    // RX/TX are members of the pair, not individually allocated objects.
    delete pCharacteristicPair;
    pCharacteristicPair = nullptr;
    delete pService;
    pNotifyDescriptor = nullptr;
    pRxCharacteristic = pTxCharacteristic = nullptr;
    pService = nullptr;
    service_created = false;
    gatt_rebuild_needed = true;
  }
  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
    power_error = esp_bluedroid_disable();
    if (power_error != ESP_OK) return;
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    power_error = esp_bt_controller_disable();
    if (power_error != ESP_OK) return;
  }
  controller_suspended = esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED;
#endif
}

bool SerialBLEInterface::resumeController()
{
#if defined(HELTEC_V4_COMPANION_LIGHT_SLEEP) && HELTEC_V4_COMPANION_LIGHT_SLEEP
  if (!controller_suspended && !gatt_rebuild_needed) return true;
  if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
    power_error = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (power_error != ESP_OK) return false;
  }
  if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
    power_error = esp_bluedroid_enable();
    if (power_error != ESP_OK) return false;
  }
  // Re-register the application and rebuild its encrypted GATT attributes.
  app_registered = false;
  power_error = esp_ble_gatts_app_register(pServer->m_appId);
  if (power_error != ESP_OK) return false;
  const uint32_t deadline = millis() + 2000;
  while (!app_registered && static_cast<int32_t>(millis() - deadline) < 0) delay(1);
  if (!app_registered) { power_error = ESP_ERR_TIMEOUT; return false; }
  esp_ble_gap_set_device_name(saved_device_name);
  BLEDevice::setMTU(MAX_FRAME_SIZE);
  BLESecurity sec;
  sec.setStaticPIN(_pin_code);
  sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  createService();
  gatt_rebuild_needed = false;
  controller_suspended = false;
#endif
  return true;
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
