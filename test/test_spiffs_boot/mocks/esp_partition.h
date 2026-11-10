#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_FAIL = -1;
using esp_partition_type_t = int;
using esp_partition_subtype_t = int;
constexpr esp_partition_type_t ESP_PARTITION_TYPE_DATA = 1;
constexpr esp_partition_subtype_t ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 0x82;

struct esp_partition_t {
  uint32_t size = 0;
  bool encrypted = false;
};

inline esp_partition_t g_storage_partition;
inline bool g_storage_partition_exists = true;
inline unsigned g_storage_find_calls = 0;
inline esp_partition_type_t g_storage_found_type = -1;
inline esp_partition_subtype_t g_storage_found_subtype = -1;
inline bool g_storage_lookup_label_null = false;
inline std::vector<uint8_t> g_storage_flash;
inline std::vector<std::pair<size_t, size_t>> g_storage_reads;
inline int g_storage_read_failure = -1;

inline const esp_partition_t* esp_partition_find_first(
    esp_partition_type_t type, esp_partition_subtype_t subtype, const char* label) {
  ++g_storage_find_calls;
  g_storage_found_type = type;
  g_storage_found_subtype = subtype;
  g_storage_lookup_label_null = label == nullptr;
  return g_storage_partition_exists ? &g_storage_partition : nullptr;
}

inline esp_err_t esp_partition_read(const esp_partition_t* partition, size_t offset,
                                    void* destination, size_t length) {
  g_storage_reads.emplace_back(offset, length);
  if (partition != &g_storage_partition || offset > g_storage_flash.size() ||
      length > g_storage_flash.size() - offset ||
      static_cast<int>(g_storage_reads.size()) == g_storage_read_failure) {
    return ESP_FAIL;
  }
  std::memcpy(destination, g_storage_flash.data() + offset, length);
  return ESP_OK;
}
