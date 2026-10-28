#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

// Native-only storage/SoftDevice double. All contents are constructed by the test.
#define SVC_CONTEXT_FLAG 3
#define BOND_FNAME_LEN 64
#define FO_WRITE 1
#define NRF_SUCCESS 0
#define BOND_LOG(...) do {} while (0)
#define SB_VERIFY_1(condition) SB_VERIFY_2(condition, false)
#define SB_VERIFY_2(condition, result) do { if (!(condition)) return result; } while (0)
#define SB_VERIFY_SELECT(_1, _2, name, ...) name
#define VERIFY(...) SB_VERIFY_SELECT(__VA_ARGS__, SB_VERIFY_2, SB_VERIFY_1)(__VA_ARGS__)
#define VERIFY_STATUS(status, ...) VERIFY((status) == NRF_SUCCESS, __VA_ARGS__)
struct ble_gap_addr_t { uint8_t addr[6] = {}; };
inline std::vector<uint8_t> mock_bond_file, mock_sys_attributes;
inline int mock_file_writes = 0, mock_seek_calls = 0, mock_fail_seek_call = 0;
inline struct SyntheticFS {} InternalFS;

namespace Adafruit_LittleFS_Namespace {
class File {
  uint32_t _position;
public:
  File(const char*, int, SyntheticFS&) : _position(static_cast<uint32_t>(mock_bond_file.size())) {}
  explicit operator bool() const { return true; }
  uint32_t size() const { return static_cast<uint32_t>(mock_bond_file.size()); }
  uint32_t position() const { return _position; }
  bool seek(uint32_t value) {
    ++mock_seek_calls;
    if (mock_fail_seek_call && mock_seek_calls==mock_fail_seek_call) return false;
    _position=value; return true;
  }
  int read() { return _position<size() ? mock_bond_file[_position++] : -1; }
  int read(uint8_t* destination, uint16_t length) {
    const uint32_t count=std::min<uint32_t>(length, _position<size() ? size()-_position : 0);
    if (count) std::memcpy(destination,mock_bond_file.data()+_position,count);
    _position+=count;
    return static_cast<int>(count);
  }
  void write(uint8_t value) { write(&value,1); }
  void write(const uint8_t* data, uint16_t length) {
    ++mock_file_writes;
    if (_position+length>size()) mock_bond_file.resize(_position+length);
    if (length) std::memcpy(mock_bond_file.data()+_position,data,length);
    _position+=length;
  }
  void close() {}
};
}

inline int sd_ble_gatts_sys_attr_get(uint16_t, uint8_t* destination, uint16_t* length, unsigned flags) {
  if (flags!=SVC_CONTEXT_FLAG) return 1;
  if (!destination) { *length=static_cast<uint16_t>(mock_sys_attributes.size()); return NRF_SUCCESS; }
  if (*length<mock_sys_attributes.size()) return 1;
  std::memcpy(destination,mock_sys_attributes.data(),mock_sys_attributes.size());
  *length=static_cast<uint16_t>(mock_sys_attributes.size());
  return NRF_SUCCESS;
}
inline void get_fname(char* filename, uint8_t, const uint8_t*) { std::strcpy(filename,"synthetic-bond"); }
template<typename Callback>
inline bool ada_callback(const void*, size_t, Callback callback, uint8_t role, uint16_t handle,
                         const ble_gap_addr_t* address) {
  callback(role,handle,address);
  return true;
}
