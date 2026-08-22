#include "wfrac_device_model/device_state.hpp"

namespace wfrac_device_model {
namespace {

bool is_valid_control_target(PowerState value) {
  return value == PowerState::off || value == PowerState::on;
}

bool is_valid_control_target(HvacMode value) {
  return value == HvacMode::auto_mode || value == HvacMode::cool || value == HvacMode::heat ||
         value == HvacMode::fan || value == HvacMode::dry;
}

bool is_valid_control_target(FanSetting value) {
  return value == FanSetting::auto_mode || value == FanSetting::speed1 || value == FanSetting::speed2 ||
         value == FanSetting::speed3 || value == FanSetting::speed4;
}

DeviceStateError validate_command_intent(const CommandIntent& intent) {
  return std::visit(
      [](const auto& typed_intent) -> DeviceStateError {
        using IntentType = std::decay_t<decltype(typed_intent)>;
        if constexpr (std::is_same_v<IntentType, SetPowerIntent>) {
          return is_valid_control_target(typed_intent.value) ? DeviceStateError::none
                                                              : DeviceStateError::invalid_control_target;
        } else if constexpr (std::is_same_v<IntentType, SetModeIntent>) {
          return is_valid_control_target(typed_intent.value) ? DeviceStateError::none
                                                              : DeviceStateError::invalid_control_target;
        } else if constexpr (std::is_same_v<IntentType, SetTargetTemperatureIntent>) {
          return DeviceStateError::none;
        } else {
          return is_valid_control_target(typed_intent.value) ? DeviceStateError::none
                                                              : DeviceStateError::invalid_control_target;
        }
      },
      intent);
}

}  // namespace

DeviceStateError apply_desired_intent(const CommandIntent& intent, DesiredHvacState* desired) {
  if (desired == nullptr) return DeviceStateError::invalid_argument;
  const DeviceStateError validation = validate_command_intent(intent);
  if (validation != DeviceStateError::none) return validation;

  return std::visit(
      [desired](const auto& typed_intent) -> DeviceStateError {
        using IntentType = std::decay_t<decltype(typed_intent)>;
        if constexpr (std::is_same_v<IntentType, SetPowerIntent>) {
          desired->power = typed_intent.value;
        } else if constexpr (std::is_same_v<IntentType, SetModeIntent>) {
          desired->mode = typed_intent.value;
        } else if constexpr (std::is_same_v<IntentType, SetTargetTemperatureIntent>) {
          desired->target_temperature_centi_celsius = typed_intent.value;
        } else {
          desired->fan = typed_intent.value;
        }
        return DeviceStateError::none;
      },
      intent);
}

DeviceStateError stage_desired_intent(const CommandIntent& intent, DeviceState* state) {
  if (state == nullptr) return DeviceStateError::invalid_argument;
  return apply_desired_intent(intent, &state->desired);
}

DeviceStateError begin_pending_command(const CommandIntent& intent, DeviceState* state) {
  if (state == nullptr) return DeviceStateError::invalid_argument;
  const DeviceStateError validation = validate_command_intent(intent);
  if (validation != DeviceStateError::none) return validation;
  if (state->pending.intent.has_value()) return DeviceStateError::already_pending;
  state->pending.intent = intent;
  return DeviceStateError::none;
}

DeviceStateError finish_pending_command(PendingCommandOutcome outcome, DeviceState* state) {
  if (state == nullptr) return DeviceStateError::invalid_argument;
  if (!state->pending.intent.has_value()) return DeviceStateError::no_pending_command;
  (void)outcome;
  state->pending.intent.reset();
  return DeviceStateError::none;
}

void apply_observation(const BasicHvacState& observation, DeviceState* state) {
  if (state == nullptr) return;
  state->observed.value = observation;
  state->observed.has_value = true;
}

void clear_all_desired(DeviceState* state) {
  if (state == nullptr) return;
  state->desired = DesiredHvacState{};
}

void set_availability(AvailabilityState availability, DeviceState* state) {
  if (state == nullptr) return;
  state->availability = availability;
}

}  // namespace wfrac_device_model
