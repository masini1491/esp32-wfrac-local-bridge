#pragma once

#include <cstdint>

namespace wfrac_protocol {

enum class CapabilityGroup {
  unknown,
  separate_2021,
  global_2022,
  high_end_for_japanese_2023,
  zt_2025,
  fdt_2023,
};

struct CapabilityFlags {
  bool power_consumption = false;
  bool vacant_property = false;
  bool self_clean_operation = false;
  bool wind_direction_lr = false;
  bool entrust = false;
  bool led_light = false;
  bool auto_heating = false;
  bool home_leave_mode = false;
  bool temp_information = false;
  bool indoor_and_outdoor_temp = false;
  bool preset_temp_auto = false;
  bool call_center_phone_number = false;
  bool outdoor_temp_always_show = false;
  bool outdoor_temp_outside_tokyo = false;
  bool cool_hot_judge = false;
  bool preset_temp_range_2 = false;
  bool operation_data = false;
};

struct CapabilityProfile {
  uint8_t model_nr_raw = 0;
  bool profile_known = false;
  CapabilityGroup group = CapabilityGroup::unknown;
  CapabilityFlags flags{};
};

CapabilityProfile decode_capabilities(uint8_t model_nr_raw);

}  // namespace wfrac_protocol
