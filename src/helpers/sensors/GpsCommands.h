#pragma once

#include <helpers/SensorManager.h>
#include <stdio.h>
#include <string.h>
#include "GpsCyclePrefs.h"

namespace gps {

namespace detail {
inline void gpsReply(char* reply, size_t reply_size, const char* text) {
  if (reply != nullptr && reply_size > 0) snprintf(reply, reply_size, "%s", text);
}
} // namespace detail

// No persistence or NodePrefs dependency. The caller saves only when changed is
// true. SensorManager must apply accepted configurations atomically; a rejected
// one must leave its runtime configuration untouched, just as prefs remain here.
inline bool handleGpsCommand(const char* command, SensorManager& sensors,
                             uint8_t& user_enabled, uint32_t& interval,
                             GpsCyclePrefs& cycle, bool& changed,
                             char* reply, size_t reply_size) {
  changed = false;
  if (command == nullptr) return false;
  if (strcmp(command, "get gps.cycle") == 0 || strcmp(command, "gps") == 0 ||
      strcmp(command, "gps cycle") == 0) {
    if (reply != nullptr && reply_size > 0) sensors.formatGpsCycleDiagnostics(reply, reply_size);
    return true;
  }
  // These two commands still belong to the caller's existing location policy.
  if (strcmp(command, "gps setloc") == 0 || strcmp(command, "gps advert") == 0 ||
      strncmp(command, "gps advert ", 11) == 0) return false;
  if (strncmp(command, "gps", 3) != 0 && strncmp(command, "get gps.cycle", 13) != 0) return false;

  if (strcmp(command, "gps on") == 0 || strcmp(command, "gps off") == 0) {
    const uint8_t requested = strcmp(command, "gps on") == 0 ? 1 : 0;
    if (!sensors.setSettingValue("gps", requested ? "1" : "0")) {
      detail::gpsReply(reply, reply_size, "Error: GPS toggle unavailable");
    } else {
      changed = user_enabled != requested;
      user_enabled = requested;
      detail::gpsReply(reply, reply_size, "ok");
    }
    return true;
  }
  if (strcmp(command, "gps sync") == 0) {
    if (user_enabled == 0) {
      detail::gpsReply(reply, reply_size, "Error: GPS is off");
    } else {
      detail::gpsReply(reply, reply_size, sensors.requestGpsSync() ? "ok" : "Error: GPS sync unavailable");
    }
    return true;
  }
  if (strcmp(command, "gps diag") == 0) {
    LocationProvider* provider = sensors.getLocationProvider();
    if (provider == nullptr) {
      detail::gpsReply(reply, reply_size, "Error: GPS provider unavailable");
    } else if (reply != nullptr && reply_size > 0) {
      const int prefix = snprintf(reply, reply_size, "req:%u ", user_enabled ? 1U : 0U);
      if (prefix >= 0 && static_cast<size_t>(prefix) < reply_size) {
        provider->formatDiagnostics(reply + prefix, reply_size - static_cast<size_t>(prefix));
      }
    }
    return true;
  }

  GpsDutyCycleConfig proposed = cycle.config();
  bool proposed_enabled = cycle.enabled != 0;
  uint32_t value = 0;
  enum Field { ENABLED, ACQUIRE, REST, HOLD, INTERVAL, UNKNOWN } field = UNKNOWN;
  const char* argument = nullptr;
  if (strcmp(command, "gps cycle on") == 0 || strcmp(command, "gps cycle off") == 0) {
    field = ENABLED;
    proposed_enabled = strcmp(command, "gps cycle on") == 0;
  } else if (strncmp(command, "gps acquire ", 12) == 0) {
    field = ACQUIRE; argument = command + 12;
  } else if (strncmp(command, "gps rest ", 9) == 0) {
    field = REST; argument = command + 9;
  } else if (strncmp(command, "gps hold ", 9) == 0) {
    field = HOLD; argument = command + 9;
  } else if (strncmp(command, "gps interval ", 13) == 0) {
    field = INTERVAL; argument = command + 13;
  }
  if (field == UNKNOWN || (argument != nullptr && !detail::parseGpsUnsigned(argument, value))) {
    detail::gpsReply(reply, reply_size, "Error: unknown GPS command or invalid decimal value");
    return true;
  }
  if (field == INTERVAL) {
    if (value > 86400) {
      detail::gpsReply(reply, reply_size, "Error: GPS interval must be 0..86400 seconds");
      return true;
    }
    char runtime_interval[11];
    snprintf(runtime_interval, sizeof(runtime_interval), "%lu", static_cast<unsigned long>(value ? value : 1U));
    if (!sensors.setSettingValue("gps_interval", runtime_interval)) {
      detail::gpsReply(reply, reply_size, "Error: GPS interval unavailable");
    } else {
      changed = interval != value;
      interval = value;
      detail::gpsReply(reply, reply_size, "ok");
    }
    return true;
  }
  if (field == ACQUIRE) proposed.acquire_seconds = value;
  else if (field == REST) proposed.rest_seconds = value;
  else if (field == HOLD) proposed.fix_hold_seconds = value;
  GpsDutyCyclePolicy validator;
  if (!validator.configure(proposed)) {
    detail::gpsReply(reply, reply_size, "Error: acquire 5..3600s, rest 1..86400s, hold 0..60s and <= acquire");
    return true;
  }
  if (!sensors.configureGpsCycle(proposed, proposed_enabled)) {
    detail::gpsReply(reply, reply_size, "Error: GPS cycle configuration rejected");
    return true;
  }
  changed = cycle.enabled != static_cast<uint8_t>(proposed_enabled) ||
            cycle.acquire != proposed.acquire_seconds || cycle.rest != proposed.rest_seconds ||
            cycle.hold != proposed.fix_hold_seconds;
  cycle.enabled = proposed_enabled ? 1 : 0;
  cycle.acquire = proposed.acquire_seconds;
  cycle.rest = proposed.rest_seconds;
  cycle.hold = proposed.fix_hold_seconds;
  detail::gpsReply(reply, reply_size, "ok");
  return true;
}

} // namespace gps
