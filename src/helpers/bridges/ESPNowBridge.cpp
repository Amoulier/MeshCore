#include "ESPNowBridge.h"

#include <WiFi.h>
#include <esp_wifi.h>

#ifdef WITH_ESPNOW_BRIDGE

#ifndef HELTEC_V4_ESPNOW_ACTIVE_MSEC
#define HELTEC_V4_ESPNOW_ACTIVE_MSEC 5000UL
#endif
#ifndef HELTEC_V4_ESPNOW_SLEEP_MSEC
#define HELTEC_V4_ESPNOW_SLEEP_MSEC 25000UL
#endif

ESPNowBridge *ESPNowBridge::_instance = nullptr;

void ESPNowBridge::recv_cb(const uint8_t *mac, const uint8_t *data, int32_t len)
{
  if (_instance) _instance->onDataRecv(mac, data, len);
}

void ESPNowBridge::send_cb(const uint8_t *mac, esp_now_send_status_t status)
{
  if (_instance) _instance->onDataSent(mac, status);
}

ESPNowBridge::ESPNowBridge(NodePrefs *prefs, mesh::PacketManager *mgr, mesh::RTCClock *rtc)
    : BridgeBase(prefs, mgr, rtc), _rx_buffer_pos(0),
      _duty_cycle_deadline(0), _duty_cycle_sleeping(false)
{
  _instance = this;
}

void ESPNowBridge::extendDutyCycleWindow()
{
#if defined(HELTEC_V4_ESPNOW_DUTY_CYCLE) && HELTEC_V4_ESPNOW_DUTY_CYCLE
  _duty_cycle_sleeping = false;
  _duty_cycle_deadline = millis() + HELTEC_V4_ESPNOW_ACTIVE_MSEC;
#endif
}

void ESPNowBridge::scheduleDutyCycleSleep()
{
#if defined(HELTEC_V4_ESPNOW_DUTY_CYCLE) && HELTEC_V4_ESPNOW_DUTY_CYCLE
  _duty_cycle_sleeping = true;
  _duty_cycle_deadline = millis() + HELTEC_V4_ESPNOW_SLEEP_MSEC;
#endif
}

void ESPNowBridge::begin()
{
  if (_initialized) return;

  BRIDGE_DEBUG_PRINTLN("Initializing...\n");
  _duty_cycle_sleeping = false;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);

  if (esp_wifi_set_channel(_prefs->bridge_channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    BRIDGE_DEBUG_PRINTLN("Error setting WIFI channel to %d\n", _prefs->bridge_channel);
    WiFi.mode(WIFI_OFF);
    scheduleDutyCycleSleep();
    return;
  }

  if (esp_now_init() != ESP_OK) {
    BRIDGE_DEBUG_PRINTLN("Error initializing ESP-NOW\n");
    WiFi.mode(WIFI_OFF);
    scheduleDutyCycleSleep();
    return;
  }

  esp_now_register_recv_cb(recv_cb);
  esp_now_register_send_cb(send_cb);

  esp_now_peer_info_t peerInfo = {};
  memset(peerInfo.peer_addr, 0xFF, ESP_NOW_ETH_ALEN);
  peerInfo.channel = _prefs->bridge_channel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    BRIDGE_DEBUG_PRINTLN("Failed to add broadcast peer\n");
    esp_now_register_recv_cb(nullptr);
    esp_now_register_send_cb(nullptr);
    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    scheduleDutyCycleSleep();
    return;
  }

  _initialized = true;
  extendDutyCycleWindow();
}

void ESPNowBridge::end()
{
  BRIDGE_DEBUG_PRINTLN("Stopping...\n");

  if (_initialized) {
    uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    esp_now_del_peer(broadcastAddress);
    esp_now_register_recv_cb(nullptr);
    esp_now_register_send_cb(nullptr);
    esp_now_deinit();
  }

  WiFi.mode(WIFI_OFF);
  _initialized = false;
}

void ESPNowBridge::loop()
{
#if defined(HELTEC_V4_ESPNOW_DUTY_CYCLE) && HELTEC_V4_ESPNOW_DUTY_CYCLE
  if (!_prefs->bridge_enabled) return;

  const unsigned long now = millis();
  if (_duty_cycle_sleeping) {
    if (static_cast<long>(now - _duty_cycle_deadline) >= 0) begin();
    return;
  }

  if (_initialized && _duty_cycle_deadline &&
      static_cast<long>(now - _duty_cycle_deadline) >= 0) {
    end();
    scheduleDutyCycleSleep();
  } else if (!_initialized) {
    scheduleDutyCycleSleep();
  }
#endif
}

void ESPNowBridge::xorCrypt(uint8_t *data, size_t len)
{
  const size_t keyLen = strlen(_prefs->bridge_secret);
  if (keyLen == 0) return;
  for (size_t i = 0; i < len; i++) {
    data[i] ^= _prefs->bridge_secret[i % keyLen];
  }
}

void ESPNowBridge::onDataRecv(const uint8_t *mac, const uint8_t *data, int32_t len)
{
  (void)mac;

  if (len < static_cast<int32_t>(BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE)) {
    BRIDGE_DEBUG_PRINTLN("RX packet too small, len=%d\n", len);
    return;
  }
  if (len > static_cast<int32_t>(MAX_ESPNOW_PACKET_SIZE)) {
    BRIDGE_DEBUG_PRINTLN("RX packet too large, len=%d\n", len);
    return;
  }

  const uint16_t received_magic = (data[0] << 8) | data[1];
  if (received_magic != BRIDGE_PACKET_MAGIC) {
    BRIDGE_DEBUG_PRINTLN("RX invalid magic 0x%04X\n", received_magic);
    return;
  }

  extendDutyCycleWindow();

  uint8_t decrypted[MAX_ESPNOW_PACKET_SIZE];
  const size_t encryptedDataLen = len - BRIDGE_MAGIC_SIZE;
  memcpy(decrypted, data + BRIDGE_MAGIC_SIZE, encryptedDataLen);
  xorCrypt(decrypted, encryptedDataLen);

  const uint16_t received_checksum = (decrypted[0] << 8) | decrypted[1];
  const size_t payloadLen = encryptedDataLen - BRIDGE_CHECKSUM_SIZE;
  if (!validateChecksum(decrypted + BRIDGE_CHECKSUM_SIZE, payloadLen, received_checksum)) {
    BRIDGE_DEBUG_PRINTLN("RX checksum mismatch, rcv=0x%04X\n", received_checksum);
    return;
  }

  BRIDGE_DEBUG_PRINTLN("RX, payload_len=%d\n", payloadLen);
  mesh::Packet *pkt = _mgr->allocNew();
  if (!pkt) return;

  if (pkt->readFrom(decrypted + BRIDGE_CHECKSUM_SIZE, payloadLen)) {
    onPacketReceived(pkt);
  } else {
    _mgr->free(pkt);
  }
}

void ESPNowBridge::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  (void)mac_addr;
  if (status == ESP_NOW_SEND_SUCCESS) extendDutyCycleWindow();
}

void ESPNowBridge::sendPacket(mesh::Packet *packet)
{
  if (!packet) {
    BRIDGE_DEBUG_PRINTLN("TX invalid packet pointer\n");
    return;
  }

#if defined(HELTEC_V4_ESPNOW_DUTY_CYCLE) && HELTEC_V4_ESPNOW_DUTY_CYCLE
  // Outbound MeshCore traffic wakes the bridge immediately; only inbound
  // ESP-NOW traffic is constrained to the documented listening windows.
  if (!_initialized && _prefs->bridge_enabled) begin();
#endif
  if (!_initialized) return;

  extendDutyCycleWindow();

  if (_seen_packets.wasSeen(packet)) return;
  _seen_packets.markSeen(packet);

  uint8_t sizingBuffer[MAX_PAYLOAD_SIZE];
  const uint16_t meshPacketLen = packet->writeTo(sizingBuffer);
  if (meshPacketLen > MAX_PAYLOAD_SIZE) {
    BRIDGE_DEBUG_PRINTLN("TX packet too large (payload=%d, max=%d)\n",
                         meshPacketLen, MAX_PAYLOAD_SIZE);
    return;
  }

  uint8_t buffer[MAX_ESPNOW_PACKET_SIZE];
  buffer[0] = (BRIDGE_PACKET_MAGIC >> 8) & 0xFF;
  buffer[1] = BRIDGE_PACKET_MAGIC & 0xFF;

  const size_t packetOffset = BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE;
  memcpy(buffer + packetOffset, sizingBuffer, meshPacketLen);

  const uint16_t checksum = fletcher16(buffer + packetOffset, meshPacketLen);
  buffer[2] = (checksum >> 8) & 0xFF;
  buffer[3] = checksum & 0xFF;
  xorCrypt(buffer + BRIDGE_MAGIC_SIZE, meshPacketLen + BRIDGE_CHECKSUM_SIZE);

  const size_t totalPacketSize = BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE + meshPacketLen;
  const uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  const esp_err_t result = esp_now_send(broadcastAddress, buffer, totalPacketSize);

  if (result == ESP_OK) {
    BRIDGE_DEBUG_PRINTLN("TX, len=%d\n", meshPacketLen);
  } else {
    BRIDGE_DEBUG_PRINTLN("TX FAILED!\n");
  }
}

void ESPNowBridge::onPacketReceived(mesh::Packet *packet)
{
  handleReceivedPacket(packet);
}

#endif
