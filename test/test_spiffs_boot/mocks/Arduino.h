#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class MockStorageHalt : public std::runtime_error {
public:
  MockStorageHalt() : std::runtime_error("stopped at storage diagnostic wait") {}
};

inline unsigned g_storage_delay_calls = 0;
inline uint32_t g_storage_delay_ms = 0;

inline void delay(uint32_t ms) {
  ++g_storage_delay_calls;
  g_storage_delay_ms = ms;
  throw MockStorageHalt();
}

struct MockStorageSerial {
  unsigned begin_calls = 0;
  std::vector<std::string> messages;
  void begin(unsigned long) { ++begin_calls; }
  void println(const char* text) { messages.emplace_back(text); }
  void print(const char* text) { messages.emplace_back(text); }
  void flush() {}
  explicit operator bool() const { return true; }
};

inline MockStorageSerial Serial;
