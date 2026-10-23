#pragma once

#include <math.h>
#include <stdint.h>

namespace heltec_v4 {

enum class PowerProfile : uint8_t {
  Standard = 0,
  SolarRepeater = 1
};

enum class RfPowerClass : uint8_t {
  LowPower22 = 22,
  HighPower28 = 28
};

constexpr int8_t LOW_POWER_OUTPUT_DBM = 22;
constexpr int8_t HIGH_POWER_OUTPUT_DBM = 28;

static const uint16_t STANDARD_OCV_MILLIVOLTS[11] = {
  4300, 4160, 4080, 4000, 3920, 3825, 3720, 3630, 3530, 3420, 3100
};

static const uint16_t SOLAR_OCV_MILLIVOLTS[11] = {
  4300, 4160, 4080, 4000, 3920, 3825, 3750, 3690, 3630, 3565, 3500
};

// Gain tables published by Meshtastic/Heltec for the V4 front ends. They are
// also used to estimate antenna-path output after the manufacturer-specific
// input-power conversion has been applied.
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

constexpr bool isBatteryRecoveryRadioStateKnownSafe(bool recovery_latched, bool timer_wake)
{
  return recovery_latched && timer_wake;
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
  if (elapsed_millis < step_interval_millis) {
    return reported_percent;
  }
  return target_percent > reported_percent
           ? static_cast<uint8_t>(reported_percent + 1)
           : static_cast<uint8_t>(reported_percent - 1);
}

inline int8_t clampRadioInput(int16_t radio_input_dbm, int8_t maximum_input_dbm = 22)
{
  if (radio_input_dbm < -9) {
    return -9;
  }
  if (radio_input_dbm > maximum_input_dbm) {
    return maximum_input_dbm;
  }
  return static_cast<int8_t>(radio_input_dbm);
}

inline int8_t radioInputPowerForRequestedOutput(int8_t requested_output_dbm,
                                                const uint8_t (&gain_table)[22])
{
  for (int8_t radio_dbm = 0; radio_dbm < 22; radio_dbm++) {
    const int16_t estimated_output = radio_dbm + gain_table[radio_dbm];
    const bool exceeded_request = estimated_output > requested_output_dbm;
    const bool reached_last_point = radio_dbm == 21 && estimated_output <= requested_output_dbm;
    if (exceeded_request || reached_last_point) {
      return clampRadioInput(static_cast<int16_t>(requested_output_dbm) - gain_table[radio_dbm]);
    }
  }

  return clampRadioInput(requested_output_dbm);
}

// Heltec's current V4 GC1109 driver converts requested antenna-path output to
// SX1262 input with this measured cubic curve, then clamps the radio to -9..20
// dBm. In particular, a requested 28 dBm maps to 19 dBm at the SX1262.
inline int8_t gc1109RadioInputPower(int8_t requested_output_dbm)
{
  const float x = static_cast<float>(requested_output_dbm);
  const float converted = 0.0004f * x * x * x - 0.011f * x * x + 1.0866f * x - 11.365f;
  return clampRadioInput(static_cast<int16_t>(floorf(converted)), 20);
}

inline int8_t kct8103lRadioInputPower(int8_t requested_output_dbm)
{
  return radioInputPowerForRequestedOutput(requested_output_dbm, KCT8103L_TX_GAIN_DB);
}

inline int8_t lowPowerRadioInputPower(int8_t requested_output_dbm)
{
  return clampRadioInput(requested_output_dbm, LOW_POWER_OUTPUT_DBM);
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
  requested_radio_dbm = clampRadioInput(requested_radio_dbm);
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

inline bool isFrequencyCompatibleWithPowerClass(float frequency_mhz, RfPowerClass power_class)
{
  return power_class == RfPowerClass::HighPower28
           ? frequency_mhz >= 863.0f && frequency_mhz <= 928.0f
           : frequency_mhz >= 433.0f && frequency_mhz <= 510.0f;
}

inline const char *rfPowerClassName(RfPowerClass power_class)
{
  return power_class == RfPowerClass::HighPower28 ? "high-power" : "low-power";
}

} // namespace heltec_v4
