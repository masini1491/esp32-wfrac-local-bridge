#pragma once

namespace wfrac_device_model {

enum class CapabilityHint { unknown, advertised_false, advertised_true };

struct CapabilityHints {
  CapabilityHint power_consumption = CapabilityHint::unknown;
  CapabilityHint vacant_property = CapabilityHint::unknown;
  CapabilityHint self_clean_operation = CapabilityHint::unknown;
  CapabilityHint wind_direction_lr = CapabilityHint::unknown;
  CapabilityHint entrust = CapabilityHint::unknown;
  CapabilityHint led_light = CapabilityHint::unknown;
  CapabilityHint auto_heating = CapabilityHint::unknown;
  CapabilityHint home_leave_mode = CapabilityHint::unknown;
  CapabilityHint temp_information = CapabilityHint::unknown;
  CapabilityHint indoor_and_outdoor_temp = CapabilityHint::unknown;
  CapabilityHint preset_temp_auto = CapabilityHint::unknown;
  CapabilityHint call_center_phone_number = CapabilityHint::unknown;
  CapabilityHint outdoor_temp_always_show = CapabilityHint::unknown;
  CapabilityHint outdoor_temp_outside_tokyo = CapabilityHint::unknown;
  CapabilityHint cool_hot_judge = CapabilityHint::unknown;
  CapabilityHint preset_temp_range_2 = CapabilityHint::unknown;
  CapabilityHint operation_data = CapabilityHint::unknown;
};

}  // namespace wfrac_device_model
