#pragma once

#include <stdint.h>

namespace heltec_v4 {

enum class PowerProfile : uint8_t {
  Standard = 0,
  SolarRepeater = 1
};

static const uint16_t STANDARD_OCV_MILLIVOLTS[11] = {
  4300, 4160, 4080, 4000, 3920, 3825, 3720, 3630, 3530, 3420, 3100
};

static const uint16_t SOLAR_OCV_MILLIVOLTS[11] = {
  4300, 4160, 4080, 4000, 3920, 3825, 3750, 3690, 3630, 3565, 3500
};

static const uint8_t GC1109_TX_GAIN_DB[22] = {
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 10, 10, 9, 9, 8, 7
};

static const uint8_t KCT8103L_TX_GAIN_DB[22] = {
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 12, 12, 11, 11, 10, 9, 8, 7
};

constexpr bool isPlausibleBatteryReading(uint16_t battery_millivolts,
                                         uint16_t boot_guard_min_millivolts)
{
  return battery_millivolts >= boot_guard_min_millivolts;
}

constexpr bool shouldUseCriticalBatteryRecovery(uint16_t battery_millivolts,
                                                bool recovery_latched,
                                                uint16_t boot_guard_min_millivolts,
                                                uint16_t critical_millivolts,
                                                uint16_t recovery_millivolts)
{
  return recovery_latched
           ? battery_millivolts < recovery_millivolts
           : (isPlausibleBatteryReading(battery_millivolts, boot_guard_min_millivolts) &&
              battery_millivolts <= critical_millivolts);
}

inline uint8_t updateLowReadingCounter(uint8_t current_count,
                                      uint16_t battery_millivolts,
                                      uint16_t boot_guard_min_millivolts,
                                      uint16_t critical_millivolts,
                                      uint8_t required_readings)
{
  if (!isPlausibleBatteryReading(battery_millivolts, boot_guard_min_millivolts) ||
      battery_millivolts > critical_millivolts) {
    return 0;
  }
  return current_count < required_readings ? current_count + 1 : required_readings;
}

inline uint8_t batteryPercentFromCurve(uint16_t battery_millivolts,
                                      const uint16_t (&curve)[11])
{
  if (battery_millivolts >= curve[0]) {
    return 100;
  }
  if (battery_millivolts <= curve[10]) {
    return 0;
  }

  for (uint8_t index = 0; index < 10; index++) {
    const uint16_t upper_mv = curve[index];
    const uint16_t lower_mv = curve[index + 1];
    if (battery_millivolts >= lower_mv) {
      const uint8_t lower_percent = 90 - (index * 10);
      const uint16_t span_mv = upper_mv - lower_mv;
      const uint16_t above_lower_mv = battery_millivolts - lower_mv;
      return lower_percent + static_cast<uint8_t>((above_lower_mv * 10U + span_mv / 2U) / span_mv);
    }
  }

  return 0;
}

inline uint8_t batteryPercent(uint16_t battery_millivolts, PowerProfile profile)
{
  return profile == PowerProfile::SolarRepeater
           ? batteryPercentFromCurve(battery_millivolts, SOLAR_OCV_MILLIVOLTS)
           : batteryPercentFromCurve(battery_millivolts, STANDARD_OCV_MILLIVOLTS);
}

inline uint8_t slewBatteryPercent(uint8_t reported_percent,
                                  uint8_t target_percent,
                                  uint32_t elapsed_millis,
                                  uint32_t step_interval_millis)
{
  if (reported_percent == target_percent || step_interval_millis == 0) {
    return target_percent;
  }

  const uint32_t steps = elapsed_millis / step_interval_millis;
  if (steps == 0) {
    return reported_percent;
  }

  if (target_percent > reported_percent) {
    const uint32_t candidate = static_cast<uint32_t>(reported_percent) + steps;
    return candidate >= target_percent ? target_percent : static_cast<uint8_t>(candidate);
  }

  return steps >= static_cast<uint32_t>(reported_percent - target_percent)
           ? target_percent
           : static_cast<uint8_t>(reported_percent - steps);
}

inline int8_t radioInputPowerForRequestedOutput(int8_t requested_output_dbm,
                                                const uint8_t (&gain_table)[22])
{
  for (int8_t radio_dbm = 0; radio_dbm < 22; radio_dbm++) {
    const int16_t estimated_output = radio_dbm + gain_table[radio_dbm];
    const bool exceeded_request = estimated_output > requested_output_dbm;
    const bool reached_last_point = radio_dbm == 21 && estimated_output <= requested_output_dbm;
    if (exceeded_request || reached_last_point) {
      int16_t radio_input = requested_output_dbm - gain_table[radio_dbm];
      if (radio_input < -9) {
        radio_input = -9;
      } else if (radio_input > 22) {
        radio_input = 22;
      }
      return static_cast<int8_t>(radio_input);
    }
  }

  return requested_output_dbm;
}

inline int8_t gc1109RadioInputPower(int8_t requested_output_dbm)
{
  return radioInputPowerForRequestedOutput(requested_output_dbm, GC1109_TX_GAIN_DB);
}

inline int8_t kct8103lRadioInputPower(int8_t requested_output_dbm)
{
  return radioInputPowerForRequestedOutput(requested_output_dbm, KCT8103L_TX_GAIN_DB);
}

inline int8_t estimatedOutputPowerForRadioInput(int8_t radio_input_dbm,
                                                const uint8_t (&gain_table)[22])
{
  int8_t table_index = radio_input_dbm;
  if (table_index < 0) {
    table_index = 0;
  } else if (table_index > 21) {
    table_index = 21;
  }
  return static_cast<int8_t>(radio_input_dbm + gain_table[table_index]);
}

inline int8_t maximumRadioInputForOutputLimit(int8_t output_limit_dbm,
                                              const uint8_t (&gain_table)[22])
{
  int8_t maximum_safe_input = -9;
  for (int8_t radio_input = -9; radio_input <= 22; radio_input++) {
    if (estimatedOutputPowerForRadioInput(radio_input, gain_table) <= output_limit_dbm) {
      maximum_safe_input = radio_input;
    }
  }
  return maximum_safe_input;
}

inline int8_t clampRadioInputForOutputLimit(int8_t requested_radio_dbm,
                                            int8_t output_limit_dbm,
                                            const uint8_t (&gain_table)[22])
{
  if (requested_radio_dbm < -9) {
    requested_radio_dbm = -9;
  } else if (requested_radio_dbm > 22) {
    requested_radio_dbm = 22;
  }

  const int8_t maximum_safe_input = maximumRadioInputForOutputLimit(output_limit_dbm, gain_table);
  return requested_radio_dbm > maximum_safe_input ? maximum_safe_input : requested_radio_dbm;
}

inline int8_t gc1109EstimatedOutput(int8_t radio_input_dbm)
{
  return estimatedOutputPowerForRadioInput(radio_input_dbm, GC1109_TX_GAIN_DB);
}

inline int8_t kct8103lEstimatedOutput(int8_t radio_input_dbm)
{
  return estimatedOutputPowerForRadioInput(radio_input_dbm, KCT8103L_TX_GAIN_DB);
}

inline int8_t clampGc1109RadioInput(int8_t requested_radio_dbm, int8_t output_limit_dbm)
{
  return clampRadioInputForOutputLimit(requested_radio_dbm, output_limit_dbm, GC1109_TX_GAIN_DB);
}

inline int8_t clampKct8103lRadioInput(int8_t requested_radio_dbm, int8_t output_limit_dbm)
{
  return clampRadioInputForOutputLimit(requested_radio_dbm, output_limit_dbm, KCT8103L_TX_GAIN_DB);
}

} // namespace heltec_v4
