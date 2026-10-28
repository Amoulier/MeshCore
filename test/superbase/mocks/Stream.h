#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>
class Stream {
public:
  virtual ~Stream() = default;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual size_t write(const uint8_t*, size_t) = 0;
};
class MockUSBSerial : public Stream {
public:
  bool connected = false;
  std::vector<uint8_t> input, output;
  size_t offset = 0;
  explicit operator bool() const { return connected; }
  int available() override { return input.size() - offset; }
  int read() override { return available() ? input[offset++] : -1; }
  size_t write(const uint8_t* bytes, size_t len) override { output.insert(output.end(), bytes, bytes+len); return len; }
};
