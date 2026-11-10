#pragma once

#include <stdint.h>

namespace gps {

// Seconds are persisted by callers; this helper uses only wrap-safe monotonic
// millisecond differences. It never reads a clock or operates hardware.
struct GpsDutyCycleConfig {
  uint32_t acquire_seconds;
  uint32_t rest_seconds;
  uint32_t fix_hold_seconds;

  GpsDutyCycleConfig(uint32_t acquire = 120, uint32_t rest = 900, uint32_t fix_hold = 5)
      : acquire_seconds(acquire), rest_seconds(rest), fix_hold_seconds(fix_hold) {}
};

class GpsDutyCyclePolicy {
public:
  enum class State : uint8_t { Off, Continuous, Acquiring, Resting };
  enum class Action : uint8_t { None, Start, Stop };
  enum : uint32_t { NO_TRANSITION = UINT32_MAX };

private:
  GpsDutyCycleConfig _config;
  State _state = State::Off;
  uint32_t _phase_started_ms = 0;
  uint32_t _cycle_fix_sequence = 0;
  uint32_t _fix_started_ms = 0;
  bool _holding_fix = false;

  static uint32_t elapsed(uint32_t now, uint32_t start) {
    return static_cast<uint32_t>(now - start);
  }

  static uint32_t remaining(uint32_t duration, uint32_t elapsed_ms) {
    return elapsed_ms >= duration ? 0 : duration - elapsed_ms;
  }

  void beginAcquisition(uint32_t now_ms, uint32_t fix_sequence) {
    _state = State::Acquiring;
    _phase_started_ms = now_ms;
    _cycle_fix_sequence = fix_sequence;
    _holding_fix = false;
  }

public:
  // False means the entire configuration is rejected and the old one remains.
  // Changing periods never resets phase/fix elapsed time or operates hardware.
  bool configure(const GpsDutyCycleConfig& config) {
    if (config.acquire_seconds < 5 || config.acquire_seconds > 3600 ||
        config.rest_seconds < 1 || config.rest_seconds > 86400 ||
        config.fix_hold_seconds > 60 || config.fix_hold_seconds > config.acquire_seconds) {
      return false;
    }
    _config = config;
    return true;
  }

  const GpsDutyCycleConfig& config() const { return _config; }
  State state() const { return _state; }
  bool receiverOn() const { return _state == State::Continuous || _state == State::Acquiring; }

  // user_enabled is the owner's GPS request, independent of the physical rail.
  // power_saving_enabled is a separate opt-in (legacy/default must be false).
  // fix_sequence must advance ONLY for a newly parsed valid positioning fix,
  // e.g. GGA/RMC, not merely cached validity, RTC sync or a telemetry request.
  // A caller must preserve/cache that fix before applying a returned Stop.
  Action update(uint32_t now_ms, bool user_enabled, bool power_saving_enabled,
                bool fix_valid, uint32_t fix_sequence) {
    const bool was_on = receiverOn();
    if (!user_enabled) {
      _state = State::Off;
      _holding_fix = false;
      return was_on ? Action::Stop : Action::None;
    }
    if (!power_saving_enabled) {
      _state = State::Continuous;
      _holding_fix = false;
      return was_on ? Action::None : Action::Start;
    }
    if (_state == State::Off || _state == State::Continuous) {
      beginAcquisition(now_ms, fix_sequence);
      return was_on ? Action::None : Action::Start;
    }
    if (_state == State::Resting) {
      if (elapsed(now_ms, _phase_started_ms) >= _config.rest_seconds * 1000U) {
        beginAcquisition(now_ms, fix_sequence);
        return Action::Start;
      }
      return Action::None;
    }

    // Ignore a valid fix left in the parser from before this acquisition.
    if (!fix_valid) {
      _holding_fix = false;
    } else if (fix_sequence != _cycle_fix_sequence && !_holding_fix) {
      _fix_started_ms = now_ms;
      _holding_fix = true;
    }
    const bool acquisition_expired = elapsed(now_ms, _phase_started_ms) >= _config.acquire_seconds * 1000U;
    const bool fix_ready = _holding_fix && elapsed(now_ms, _fix_started_ms) >= _config.fix_hold_seconds * 1000U;
    if (acquisition_expired || fix_ready) {
      _state = State::Resting;
      _phase_started_ms = now_ms;
      _holding_fix = false;
      return Action::Stop;
    }
    return Action::None;
  }

  // Observation only: telemetry/status reads can inspect this as often as they
  // like. Only update() can schedule a transition; these getters never wake GPS.
  uint32_t millisUntilTransition(uint32_t now_ms) const {
    if (_state == State::Resting) {
      return remaining(_config.rest_seconds * 1000U, elapsed(now_ms, _phase_started_ms));
    }
    if (_state == State::Acquiring) {
      const uint32_t timeout = remaining(_config.acquire_seconds * 1000U,
                                         elapsed(now_ms, _phase_started_ms));
      if (_holding_fix) {
        const uint32_t hold = remaining(_config.fix_hold_seconds * 1000U,
                                        elapsed(now_ms, _fix_started_ms));
        return hold < timeout ? hold : timeout;
      }
      return timeout;
    }
    return NO_TRANSITION;
  }
};

} // namespace gps
