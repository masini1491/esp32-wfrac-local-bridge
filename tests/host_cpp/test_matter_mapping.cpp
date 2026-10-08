#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <type_traits>
#include <variant>

#include "wfrac_device_model/extended_observation.hpp"
#include "wfrac_matter_adapter/rac_mapping.hpp"

namespace dm = wfrac_device_model;
namespace ma = wfrac_matter_adapter;

static dm::DeviceState observed_state() {
  dm::DeviceState state{};
  state.availability = dm::AvailabilityState::available;
  state.observed.has_value = true;
  state.observed.value = {dm::PowerState::on, dm::HvacMode::cool, 2350, dm::FanSetting::speed3, 2325};
  return state;
}

static void projection_tests() {
  auto state = observed_state();
  // Deliberately disagreeing desired/pending must never become observed attributes.
  state.desired.power = dm::PowerState::off;
  state.desired.mode = dm::HvacMode::heat;
  state.desired.target_temperature_centi_celsius = 2700;
  state.pending.intent = dm::SetTargetTemperatureIntent{2700};
  state.pending.phase = dm::PendingCommandPhase::verifying;
  auto result = ma::project_observed_attributes(state);
  assert(result.on_off.disposition == ma::FieldDisposition::value && result.on_off.value);
  assert(result.system_mode.disposition == ma::FieldDisposition::value);
  assert(result.system_mode.value == ma::SystemMode::cool);
  assert(result.occupied_cooling_setpoint_centi_celsius.disposition == ma::FieldDisposition::value);
  assert(result.occupied_cooling_setpoint_centi_celsius.value == 2350);
  assert(result.occupied_heating_setpoint_centi_celsius.disposition == ma::FieldDisposition::withhold);
  assert(result.local_temperature_centi_celsius.disposition == ma::FieldDisposition::value);
  assert(result.local_temperature_centi_celsius.value == 2325);
  assert(result.fan == ma::FieldDisposition::unsupported);
  assert(state.desired.mode == dm::HvacMode::heat && state.pending.phase == dm::PendingCommandPhase::verifying);
  assert(std::get<dm::SetTargetTemperatureIntent>(*state.pending.intent).value == 2700);

  state.observed.value.power = dm::PowerState::off;
  result = ma::project_observed_attributes(state);
  assert(!result.on_off.value && result.system_mode.value == ma::SystemMode::off);
  assert(result.occupied_cooling_setpoint_centi_celsius.value == 2350);
  state.observed.value.power = dm::PowerState::on;
  state.observed.value.mode = dm::HvacMode::heat;
  state.observed.value.controller_room_temperature_centi_celsius = -125;
  result = ma::project_observed_attributes(state);
  assert(result.system_mode.value == ma::SystemMode::heat);
  assert(result.occupied_heating_setpoint_centi_celsius.value == 2350);
  assert(result.occupied_cooling_setpoint_centi_celsius.disposition == ma::FieldDisposition::withhold);
  assert(result.local_temperature_centi_celsius.value == -125);

  state.observed.value.power = dm::PowerState::unknown;
  result = ma::project_observed_attributes(state);
  assert(result.on_off.disposition == ma::FieldDisposition::withhold);
  assert(result.system_mode.disposition == ma::FieldDisposition::withhold);
  state.observed.value.power = dm::PowerState::on;
  state.observed.value.mode = dm::HvacMode::unknown;
  result = ma::project_observed_attributes(state);
  assert(result.system_mode.disposition == ma::FieldDisposition::withhold);
  assert(result.occupied_heating_setpoint_centi_celsius.disposition == ma::FieldDisposition::withhold);
  for (const auto mode : {dm::HvacMode::auto_mode, dm::HvacMode::fan, dm::HvacMode::dry}) {
    state.observed.value.mode = mode;
    result = ma::project_observed_attributes(state);
    assert(result.system_mode.disposition == ma::FieldDisposition::unsupported);
    assert(result.occupied_cooling_setpoint_centi_celsius.disposition == ma::FieldDisposition::unsupported);
    assert(result.occupied_heating_setpoint_centi_celsius.disposition == ma::FieldDisposition::unsupported);
  }
  for (const auto fan : {dm::FanSetting::auto_mode, dm::FanSetting::speed1, dm::FanSetting::speed2,
                         dm::FanSetting::speed3, dm::FanSetting::speed4, dm::FanSetting::unknown}) {
    state.observed.value.fan = fan;
    assert(ma::project_observed_attributes(state).fan == ma::FieldDisposition::unsupported);
  }

  state = observed_state();
  for (const auto availability : {dm::AvailabilityState::unknown, dm::AvailabilityState::unavailable}) {
    state.availability = availability;
    result = ma::project_observed_attributes(state);
    assert(result.availability == availability);
    assert(result.on_off.disposition == ma::FieldDisposition::withhold);
    assert(result.system_mode.disposition == ma::FieldDisposition::withhold);
    assert(result.occupied_cooling_setpoint_centi_celsius.disposition == ma::FieldDisposition::withhold);
    assert(result.local_temperature_centi_celsius.disposition == ma::FieldDisposition::null_value);
  }
  state.availability = dm::AvailabilityState::available;
  state.observed.has_value = false;
  assert(ma::project_observed_attributes(state).local_temperature_centi_celsius.disposition ==
         ma::FieldDisposition::null_value);
  assert(ma::project_observed_attributes(state).on_off.disposition == ma::FieldDisposition::withhold);

  state = observed_state();
  state.observed.value.target_temperature_centi_celsius = std::numeric_limits<int16_t>::min();
  state.observed.value.controller_room_temperature_centi_celsius = -27316;
  result = ma::project_observed_attributes(state);
  assert(result.occupied_cooling_setpoint_centi_celsius.disposition == ma::FieldDisposition::invalid_value);
  assert(result.local_temperature_centi_celsius.disposition == ma::FieldDisposition::null_value);
  for (const int16_t temperature : {int16_t{-27315}, int16_t{0}, int16_t{32767}}) {
    state.observed.value.target_temperature_centi_celsius = temperature;
    state.observed.value.controller_room_temperature_centi_celsius = temperature;
    result = ma::project_observed_attributes(state);
    assert(result.occupied_cooling_setpoint_centi_celsius.value == temperature);
    assert(result.local_temperature_centi_celsius.disposition == ma::FieldDisposition::value);
    assert(result.local_temperature_centi_celsius.value == temperature);
  }
}

static void command_tests() {
  auto state = observed_state();
  std::optional<dm::CommandIntent> intent;
  assert(ma::map_on_off_write(true, &intent) == ma::MappingError::none);
  assert(std::get<dm::SetPowerIntent>(*intent).value == dm::PowerState::on);
  assert(ma::map_on_off_write(false, &intent) == ma::MappingError::none);
  assert(std::get<dm::SetPowerIntent>(*intent).value == dm::PowerState::off);
  assert(ma::map_system_mode_write(0, state, &intent) == ma::MappingError::none);
  assert(std::get<dm::SetPowerIntent>(*intent).value == dm::PowerState::off);
  assert(ma::map_system_mode_write(3, state, &intent) == ma::MappingError::none);
  assert(std::get<dm::SetModeIntent>(*intent).value == dm::HvacMode::cool);
  assert(ma::map_system_mode_write(4, state, &intent) == ma::MappingError::none);
  assert(std::get<dm::SetModeIntent>(*intent).value == dm::HvacMode::heat);
  for (uint8_t mode : {uint8_t{1}, uint8_t{5}, uint8_t{7}, uint8_t{8}, uint8_t{255}}) {
    intent = dm::SetPowerIntent{dm::PowerState::on};
    assert(ma::map_system_mode_write(mode, state, &intent) == ma::MappingError::unsupported);
    assert(!intent);
  }
  for (const auto power : {dm::PowerState::off, dm::PowerState::unknown}) {
    state.observed.value.power = power;
    intent = dm::SetModeIntent{dm::HvacMode::cool};
    assert(ma::map_system_mode_write(4, state, &intent) == ma::MappingError::requires_power_on);
    assert(!intent);
  }
  state = observed_state();
  for (const int32_t temperature : {-27315, 0, 2325, 2350, 32767}) {
    assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_cooling, temperature, state, &intent) ==
           ma::MappingError::none);
    // 2325 is not rounded to WF's 0.5 C grid: downstream admission still owns it.
    assert(std::get<dm::SetTargetTemperatureIntent>(*intent).value == temperature);
  }
  for (const int32_t temperature : {-27316, 32768, std::numeric_limits<int32_t>::min(),
                                   std::numeric_limits<int32_t>::max()}) {
    intent = dm::SetTargetTemperatureIntent{2350};
    assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_cooling, temperature, state, &intent) ==
           ma::MappingError::invalid_temperature);
    assert(!intent);
  }
  assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_heating, 2400, state, &intent) ==
         ma::MappingError::mode_mismatch && !intent);
  state.observed.value.mode = dm::HvacMode::heat;
  assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_heating, 2400, state, &intent) ==
         ma::MappingError::none);
  assert(std::get<dm::SetTargetTemperatureIntent>(*intent).value == 2400);
  for (const auto mode : {dm::HvacMode::auto_mode, dm::HvacMode::fan, dm::HvacMode::dry, dm::HvacMode::unknown}) {
    state.observed.value.mode = mode;
    assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_heating, 2400, state, &intent) ==
           ma::MappingError::mode_mismatch && !intent);
  }
  state = observed_state();
  state.availability = dm::AvailabilityState::unavailable;
  assert(ma::map_system_mode_write(3, state, &intent) == ma::MappingError::observation_unavailable && !intent);
  assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_cooling, 2350, state, &intent) ==
         ma::MappingError::observation_unavailable && !intent);
  state.availability = dm::AvailabilityState::available;
  state.observed.has_value = false;
  assert(ma::map_system_mode_write(4, state, &intent) == ma::MappingError::observation_unavailable && !intent);
  assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_heating, 2350, state, &intent) ==
         ma::MappingError::observation_unavailable && !intent);
  assert(ma::map_setpoint_write(static_cast<ma::SetpointAttribute>(99), 2350, state, &intent) ==
         ma::MappingError::unsupported && !intent);
  assert(ma::map_on_off_write(true, nullptr) == ma::MappingError::invalid_argument);
  assert(ma::map_system_mode_write(3, state, nullptr) == ma::MappingError::invalid_argument);
  assert(ma::map_setpoint_write(ma::SetpointAttribute::occupied_cooling, 2350, state, nullptr) ==
         ma::MappingError::invalid_argument);
  // All translation calls leave authoritative state/lifecycle under DM ownership.
  assert(state.observed.value.mode == dm::HvacMode::cool);
  assert(state.availability == dm::AvailabilityState::available);
  assert(!state.desired.power && !state.pending.intent);
}

int main() {
  static_assert(ma::room_air_conditioner_device_type == 0x0072U);
  static_assert(ma::room_air_conditioner_device_version == 3U);
  static_assert(sizeof(ma::required_clusters) / sizeof(ma::required_clusters[0]) == 4U);
  static_assert(ma::requires_dead_front_behavior);
  // Extended protocol observations are intentionally not a Matter projection input.
  static_assert(!std::is_invocable_v<decltype(&ma::project_observed_attributes),
                                     const dm::ExtendedHvacObservation&>);
  // The current write surface remains exactly the admitted basic intents; no vane/diagnostic intent exists.
  static_assert(std::variant_size_v<dm::CommandIntent> == 4U);
  static_assert(std::is_same_v<std::variant_alternative_t<0U, dm::CommandIntent>, dm::SetPowerIntent>);
  static_assert(std::is_same_v<std::variant_alternative_t<1U, dm::CommandIntent>, dm::SetModeIntent>);
  static_assert(std::is_same_v<std::variant_alternative_t<2U, dm::CommandIntent>, dm::SetTargetTemperatureIntent>);
  static_assert(std::is_same_v<std::variant_alternative_t<3U, dm::CommandIntent>, dm::SetFanIntent>);
  projection_tests();
  command_tests();
  std::cout << "Matter RAC software mapping tests passed\n";
}
