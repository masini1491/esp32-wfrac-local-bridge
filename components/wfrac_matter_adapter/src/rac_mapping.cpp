#include "wfrac_matter_adapter/rac_mapping.hpp"

#include <limits>

namespace wfrac_matter_adapter {
namespace {

using wfrac_device_model::AvailabilityState;
using wfrac_device_model::HvacMode;
using wfrac_device_model::PowerState;

bool has_available_observation(const wfrac_device_model::DeviceState& state) {
  return state.availability == AvailabilityState::available && state.observed.has_value;
}

bool is_temperature_value(int32_t value) {
  // Thermostat temperature lower bound; INT16_MIN is not a publishable value.
  return value >= -27315 && value <= std::numeric_limits<int16_t>::max();
}

template <typename T>
AttributeProjection<T> present(T value) {
  return {FieldDisposition::value, value};
}

}  // namespace

RacObservedAttributes project_observed_attributes(const wfrac_device_model::DeviceState& state) {
  RacObservedAttributes output{};
  output.availability = state.availability;
  if (!has_available_observation(state)) return output;

  const auto& observed = state.observed.value;
  switch (observed.power) {
    case PowerState::off:
      output.on_off = present(false);
      output.system_mode = present(SystemMode::off);
      break;
    case PowerState::on:
      output.on_off = present(true);
      switch (observed.mode) {
        case HvacMode::cool: output.system_mode = present(SystemMode::cool); break;
        case HvacMode::heat: output.system_mode = present(SystemMode::heat); break;
        case HvacMode::unknown: break;
        default: output.system_mode.disposition = FieldDisposition::unsupported; break;
      }
      break;
    default: break;
  }

  AttributeProjection<int16_t>* target = nullptr;
  switch (observed.mode) {
    case HvacMode::cool: target = &output.occupied_cooling_setpoint_centi_celsius; break;
    case HvacMode::heat: target = &output.occupied_heating_setpoint_centi_celsius; break;
    case HvacMode::unknown: break;
    default:
      output.occupied_cooling_setpoint_centi_celsius.disposition = FieldDisposition::unsupported;
      output.occupied_heating_setpoint_centi_celsius.disposition = FieldDisposition::unsupported;
      break;
  }
  if (target != nullptr) {
    target->disposition = is_temperature_value(observed.target_temperature_centi_celsius)
                              ? FieldDisposition::value : FieldDisposition::invalid_value;
    if (target->disposition == FieldDisposition::value) {
      target->value = observed.target_temperature_centi_celsius;
    }
  }
  const auto room = observed.controller_room_temperature_centi_celsius;
  if (is_temperature_value(room)) output.local_temperature_centi_celsius = present(room);
  return output;
}

MappingError map_on_off_write(bool value, std::optional<wfrac_device_model::CommandIntent>* intent) {
  if (intent == nullptr) return MappingError::invalid_argument;
  intent->reset();
  *intent = wfrac_device_model::SetPowerIntent{value ? PowerState::on : PowerState::off};
  return MappingError::none;
}

MappingError map_system_mode_write(uint8_t value, const wfrac_device_model::DeviceState& state,
                                   std::optional<wfrac_device_model::CommandIntent>* intent) {
  if (intent == nullptr) return MappingError::invalid_argument;
  intent->reset();
  if (value == static_cast<uint8_t>(SystemMode::off)) return map_on_off_write(false, intent);
  HvacMode mode;
  if (value == static_cast<uint8_t>(SystemMode::cool)) mode = HvacMode::cool;
  else if (value == static_cast<uint8_t>(SystemMode::heat)) mode = HvacMode::heat;
  else return MappingError::unsupported;
  if (!has_available_observation(state)) return MappingError::observation_unavailable;
  if (state.observed.value.power != PowerState::on) return MappingError::requires_power_on;
  *intent = wfrac_device_model::SetModeIntent{mode};
  return MappingError::none;
}

MappingError map_setpoint_write(SetpointAttribute attribute, int32_t centi_celsius,
                                const wfrac_device_model::DeviceState& state,
                                std::optional<wfrac_device_model::CommandIntent>* intent) {
  if (intent == nullptr) return MappingError::invalid_argument;
  intent->reset();
  HvacMode mode;
  switch (attribute) {
    case SetpointAttribute::occupied_cooling: mode = HvacMode::cool; break;
    case SetpointAttribute::occupied_heating: mode = HvacMode::heat; break;
    default: return MappingError::unsupported;
  }
  if (!is_temperature_value(centi_celsius)) return MappingError::invalid_temperature;
  if (!has_available_observation(state)) return MappingError::observation_unavailable;
  if (state.observed.value.mode != mode) return MappingError::mode_mismatch;
  *intent = wfrac_device_model::SetTargetTemperatureIntent{static_cast<int16_t>(centi_celsius)};
  return MappingError::none;
}

}  // namespace wfrac_matter_adapter
