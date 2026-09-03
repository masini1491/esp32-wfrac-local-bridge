#include "wfrac_device_model/device_state.hpp"

namespace wfrac_device_model {
namespace {

enum class CommandField { power, mode, target_temperature, fan };

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

CommandField command_field(const CommandIntent& intent) {
  return std::visit(
      [](const auto& typed_intent) -> CommandField {
        using IntentType = std::decay_t<decltype(typed_intent)>;
        if constexpr (std::is_same_v<IntentType, SetPowerIntent>) {
          return CommandField::power;
        } else if constexpr (std::is_same_v<IntentType, SetModeIntent>) {
          return CommandField::mode;
        } else if constexpr (std::is_same_v<IntentType, SetTargetTemperatureIntent>) {
          return CommandField::target_temperature;
        } else {
          return CommandField::fan;
        }
      },
      intent);
}

bool intent_matches_observation(const CommandIntent& intent, const BasicHvacState& observation) {
  return std::visit(
      [&observation](const auto& typed_intent) -> bool {
        using IntentType = std::decay_t<decltype(typed_intent)>;
        if constexpr (std::is_same_v<IntentType, SetPowerIntent>) {
          return observation.power == typed_intent.value;
        } else if constexpr (std::is_same_v<IntentType, SetModeIntent>) {
          return observation.mode == typed_intent.value;
        } else if constexpr (std::is_same_v<IntentType, SetTargetTemperatureIntent>) {
          return observation.target_temperature_centi_celsius == typed_intent.value;
        } else {
          return observation.fan == typed_intent.value;
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
  state->pending.phase = PendingCommandPhase::sent;
  return DeviceStateError::none;
}

DeviceStateError mark_pending_command_accepted(DeviceState* state) {
  if (state == nullptr) return DeviceStateError::invalid_argument;
  if (!state->pending.intent.has_value()) return DeviceStateError::no_pending_command;
  if (state->pending.phase != PendingCommandPhase::sent) return DeviceStateError::invalid_argument;
  state->pending.phase = PendingCommandPhase::accepted;
  return DeviceStateError::none;
}

DeviceStateError begin_pending_verification(DeviceState* state) {
  if (state == nullptr) return DeviceStateError::invalid_argument;
  if (!state->pending.intent.has_value()) return DeviceStateError::no_pending_command;
  if (state->pending.phase != PendingCommandPhase::accepted) return DeviceStateError::invalid_argument;
  state->pending.phase = PendingCommandPhase::verifying;
  return DeviceStateError::none;
}

DeviceStateError finish_pending_command(PendingCommandOutcome outcome, DeviceState* state) {
  if (state == nullptr) return DeviceStateError::invalid_argument;
  if (!state->pending.intent.has_value()) return DeviceStateError::no_pending_command;
  (void)outcome;
  state->pending.intent.reset();
  state->pending.phase = PendingCommandPhase::none;
  return DeviceStateError::none;
}

void apply_observation(const BasicHvacState& observation, DeviceState* state) {
  if (state == nullptr) return;
  state->observed.value = observation;
  state->observed.has_value = true;
}

ReconciliationResult reconcile_observation(const BasicHvacState& observation, DeviceState* state) {
  ReconciliationResult result{};
  if (state == nullptr) return result;

  const std::optional<CommandIntent> pending_before = state->pending.intent;
  apply_observation(observation, state);

  if (pending_before.has_value()) {
    if (intent_matches_observation(*pending_before, observation)) {
      state->pending.intent.reset();
      state->pending.phase = PendingCommandPhase::none;
      result.pending = PendingReconciliation::observed_converged;
    } else {
      result.pending = PendingReconciliation::not_converged;
    }
  }

  const auto is_protected_by_active_pending = [state](CommandField field) {
    return state->pending.intent.has_value() && command_field(*state->pending.intent) == field;
  };

  if (state->desired.power.has_value() && state->desired.power == observation.power &&
      !is_protected_by_active_pending(CommandField::power)) {
    state->desired.power.reset();
    result.cleared_desired_power = true;
  }
  if (state->desired.mode.has_value() && state->desired.mode == observation.mode &&
      !is_protected_by_active_pending(CommandField::mode)) {
    state->desired.mode.reset();
    result.cleared_desired_mode = true;
  }
  if (state->desired.target_temperature_centi_celsius.has_value() &&
      state->desired.target_temperature_centi_celsius == observation.target_temperature_centi_celsius &&
      !is_protected_by_active_pending(CommandField::target_temperature)) {
    state->desired.target_temperature_centi_celsius.reset();
    result.cleared_desired_target_temperature = true;
  }
  if (state->desired.fan.has_value() && state->desired.fan == observation.fan &&
      !is_protected_by_active_pending(CommandField::fan)) {
    state->desired.fan.reset();
    result.cleared_desired_fan = true;
  }

  return result;
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
