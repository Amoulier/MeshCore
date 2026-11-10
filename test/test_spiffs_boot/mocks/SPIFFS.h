#pragma once

#include "esp_partition.h"

struct MockSpiffs {
  std::vector<bool> mount_results{false, true};
  std::vector<bool> format_on_fail_flags;
  bool format_result = true;
  unsigned format_calls = 0;

  bool begin(bool format_on_fail) {
    format_on_fail_flags.push_back(format_on_fail);
    const size_t index = format_on_fail_flags.size() - 1;
    return index < mount_results.size() && mount_results[index];
  }

  bool format() {
    ++format_calls;
    if (format_result && !g_storage_flash.empty()) {
      // An initialized filesystem is no longer an erased partition.
      g_storage_flash[0] = 0x01;
    }
    return format_result;
  }
};

inline MockSpiffs SPIFFS;
