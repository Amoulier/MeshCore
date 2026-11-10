#pragma once
#include <Arduino.h>
#include <deque>
#include <memory>
#include <vector>

namespace mock_wifi {
struct State {
  bool connected = true;
  std::deque<uint8_t> input;
  std::vector<uint8_t> output;
};
inline std::shared_ptr<State> pending;
}

class WiFiClient : public Stream {
  std::shared_ptr<mock_wifi::State> state;
public:
  WiFiClient() = default;
  explicit WiFiClient(std::shared_ptr<mock_wifi::State> value) : state(value) {}
  explicit operator bool() const { return connected(); }
  bool connected() const { return state && state->connected; }
  int available() override { return state ? static_cast<int>(state->input.size()) : 0; }
  int read() override {
    if (!available()) return -1;
    int value = state->input.front();
    state->input.pop_front();
    return value;
  }
  int read(uint8_t* bytes, size_t count) { return static_cast<int>(readBytes(bytes, count)); }
  size_t write(const uint8_t* bytes, size_t count) override {
    if (!connected()) return 0;
    state->output.insert(state->output.end(), bytes, bytes + count);
    return count;
  }
  void stop() {
    if (state) { state->connected = false; state->input.clear(); }
  }
  void inject(std::initializer_list<uint8_t> bytes) { state->input.insert(state->input.end(), bytes); }
};

class WiFiServer {
public:
  void begin(int) {}
  WiFiClient available() {
    WiFiClient next(mock_wifi::pending);
    mock_wifi::pending.reset();
    return next;
  }
};

inline WiFiClient connectMockWiFi() {
  mock_wifi::pending = std::make_shared<mock_wifi::State>();
  return WiFiClient(mock_wifi::pending);
}
