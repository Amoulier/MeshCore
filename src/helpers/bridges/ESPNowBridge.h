#pragma once

#include "MeshCore.h"
#include "esp_now.h"
#include "helpers/bridges/BridgeBase.h"

#ifdef WITH_ESPNOW_BRIDGE

/**
 * @brief Bridge implementation using ESP-NOW protocol for packet transport
 *
 * This bridge enables mesh packet transport over ESP-NOW, a connectionless communication
 * protocol provided by Espressif that allows ESP32 devices to communicate directly
 * without WiFi router infrastructure.
 *
 * Features:
 * - Broadcast-based communication (all bridges receive all packets)
 * - Network isolation using XOR encryption with shared secret
 * - Duplicate packet detection using SimpleMeshTables tracking
 * - Maximum packet size of 250 bytes (ESP-NOW limitation)
 * - Optional explicit radio duty cycle for battery deployments
 *
 * Packet Structure:
 * [2 bytes] Magic Header - Used to identify ESPNowBridge packets
 * [2 bytes] Fletcher-16 checksum of encrypted payload (calculated over payload only)
 * [246 bytes max] Encrypted payload containing the mesh packet
 *
 * The Fletcher-16 checksum is used to validate packet integrity and detect
 * corrupted or tampered packets. It's calculated over the encrypted payload
 * and provides a simple but effective way to verify packets are both
 * uncorrupted and from the same network (since the checksum is calculated
 * after encryption).
 *
 * Configuration:
 * - Define WITH_ESPNOW_BRIDGE to enable this bridge
 * - Define _prefs->bridge_secret with a string to set the network encryption key
 * - Define HELTEC_V4_ESPNOW_DUTY_CYCLE for the explicit low-power variant
 *
 * Network Isolation:
 * Multiple independent mesh networks can coexist by using different
 * _prefs->bridge_secret values. Packets encrypted with a different key will
 * fail the checksum validation and be discarded.
 */
class ESPNowBridge : public BridgeBase {
private:
  static ESPNowBridge *_instance;
  static void recv_cb(const uint8_t *mac, const uint8_t *data, int32_t len);
  static void send_cb(const uint8_t *mac, esp_now_send_status_t status);

  /**
   * ESP-NOW Protocol Structure:
   * - ESP-NOW header: 20 bytes (handled by ESP-NOW protocol)
   * - ESP-NOW payload: 250 bytes maximum
   * Total ESP-NOW packet: 270 bytes
   *
   * Our Bridge Packet Structure (must fit in ESP-NOW payload):
   * - Magic header: 2 bytes
   * - Checksum: 2 bytes
   * - Available payload: 246 bytes
   */
  static const size_t MAX_ESPNOW_PACKET_SIZE = 250;

  /** Size constants for packet parsing. */
  static const size_t MAX_PAYLOAD_SIZE = MAX_ESPNOW_PACKET_SIZE - (BRIDGE_MAGIC_SIZE + BRIDGE_CHECKSUM_SIZE);

  /** Buffer retained for compatibility with the existing bridge layout. */
  uint8_t _rx_buffer[MAX_ESPNOW_PACKET_SIZE];
  size_t _rx_buffer_pos;

  /** Explicit low-power duty-cycle state. */
  unsigned long _duty_cycle_deadline;
  bool _duty_cycle_sleeping;

  void xorCrypt(uint8_t *data, size_t len);
  void onDataRecv(const uint8_t *mac, const uint8_t *data, int32_t len);
  void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
  void extendDutyCycleWindow();
  void scheduleDutyCycleSleep();

public:
  ESPNowBridge(NodePrefs *prefs, mesh::PacketManager *mgr, mesh::RTCClock *rtc);

  /** Configures Wi-Fi station mode, ESP-NOW callbacks and the broadcast peer. */
  void begin() override;

  /** Deinitializes ESP-NOW and powers the Wi-Fi radio off. */
  void end() override;

  /** Maintains the optional explicit listen/sleep duty cycle. */
  void loop() override;

  void onPacketReceived(mesh::Packet *packet) override;
  void sendPacket(mesh::Packet *packet) override;
};

#endif
