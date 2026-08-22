#pragma once

#include <cstdint>

namespace wfrac_device_model {

enum class PowerState { off, on, unknown };
enum class HvacMode { auto_mode, cool, heat, fan, dry, unknown };
enum class FanSetting { auto_mode, speed1, speed2, speed3, speed4, unknown };

using TemperatureCentiC = int16_t;

struct BasicHvacState {
  PowerState power = PowerState::unknown;
  HvacMode mode = HvacMode::unknown;
  TemperatureCentiC target_temperature_centi_celsius = 0;
  FanSetting fan = FanSetting::unknown;
  TemperatureCentiC controller_room_temperature_centi_celsius = 0;
};

}  // namespace wfrac_device_model
