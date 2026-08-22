#include "wfrac_device_model/protocol_mapping.hpp"

namespace wfrac_device_model {
namespace {

PowerState map_power(wfrac_protocol::PowerState power) {
  switch (power) {
    case wfrac_protocol::PowerState::off: return PowerState::off;
    case wfrac_protocol::PowerState::on: return PowerState::on;
    default: return PowerState::unknown;
  }
}

HvacMode map_mode(wfrac_protocol::OperationMode mode) {
  switch (mode) {
    case wfrac_protocol::OperationMode::auto_mode: return HvacMode::auto_mode;
    case wfrac_protocol::OperationMode::cool: return HvacMode::cool;
    case wfrac_protocol::OperationMode::heat: return HvacMode::heat;
    case wfrac_protocol::OperationMode::fan: return HvacMode::fan;
    case wfrac_protocol::OperationMode::dry: return HvacMode::dry;
    default: return HvacMode::unknown;
  }
}

FanSetting map_fan(wfrac_protocol::FanMode fan) {
  switch (fan) {
    case wfrac_protocol::FanMode::auto_mode: return FanSetting::auto_mode;
    case wfrac_protocol::FanMode::speed1: return FanSetting::speed1;
    case wfrac_protocol::FanMode::speed2: return FanSetting::speed2;
    case wfrac_protocol::FanMode::speed3: return FanSetting::speed3;
    case wfrac_protocol::FanMode::speed4: return FanSetting::speed4;
    default: return FanSetting::unknown;
  }
}

CapabilityHint map_hint(bool value) {
  return value ? CapabilityHint::advertised_true : CapabilityHint::advertised_false;
}

}  // namespace

BasicHvacState map_basic_hvac_state(const wfrac_protocol::ReceiveState& state) {
  BasicHvacState result{};
  result.power = map_power(state.power);
  result.mode = map_mode(state.operation_mode);
  result.target_temperature_centi_celsius = static_cast<TemperatureCentiC>(state.target_temperature_half_degrees * 50);
  result.fan = map_fan(state.fan);
  result.controller_room_temperature_centi_celsius =
      static_cast<TemperatureCentiC>(state.controller_room_temperature_quarter_degrees * 25);
  return result;
}

CapabilityHints map_capability_hints(const wfrac_protocol::CapabilityProfile& profile) {
  CapabilityHints result{};
  if (!profile.profile_known) return result;
  result.power_consumption = map_hint(profile.flags.power_consumption);
  result.vacant_property = map_hint(profile.flags.vacant_property);
  result.self_clean_operation = map_hint(profile.flags.self_clean_operation);
  result.wind_direction_lr = map_hint(profile.flags.wind_direction_lr);
  result.entrust = map_hint(profile.flags.entrust);
  result.led_light = map_hint(profile.flags.led_light);
  result.auto_heating = map_hint(profile.flags.auto_heating);
  result.home_leave_mode = map_hint(profile.flags.home_leave_mode);
  result.temp_information = map_hint(profile.flags.temp_information);
  result.indoor_and_outdoor_temp = map_hint(profile.flags.indoor_and_outdoor_temp);
  result.preset_temp_auto = map_hint(profile.flags.preset_temp_auto);
  result.call_center_phone_number = map_hint(profile.flags.call_center_phone_number);
  result.outdoor_temp_always_show = map_hint(profile.flags.outdoor_temp_always_show);
  result.outdoor_temp_outside_tokyo = map_hint(profile.flags.outdoor_temp_outside_tokyo);
  result.cool_hot_judge = map_hint(profile.flags.cool_hot_judge);
  result.preset_temp_range_2 = map_hint(profile.flags.preset_temp_range_2);
  result.operation_data = map_hint(profile.flags.operation_data);
  return result;
}

}  // namespace wfrac_device_model
