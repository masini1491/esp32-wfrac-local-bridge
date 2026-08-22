#pragma once

#include <cassert>
#include <cstdint>

#include "wfrac_device_model/device_state.hpp"

namespace device_reconciliation_tests {

using wfrac_device_model::AvailabilityState;
using wfrac_device_model::BasicHvacState;
using wfrac_device_model::CommandIntent;
using wfrac_device_model::DeviceState;
using wfrac_device_model::DeviceStateError;
using wfrac_device_model::FanSetting;
using wfrac_device_model::HvacMode;
using wfrac_device_model::PendingReconciliation;
using wfrac_device_model::PowerState;
using wfrac_device_model::SetFanIntent;
using wfrac_device_model::SetModeIntent;
using wfrac_device_model::SetPowerIntent;
using wfrac_device_model::SetTargetTemperatureIntent;

BasicHvacState reconciliation_observation(PowerState power = PowerState::unknown,
                                          HvacMode mode = HvacMode::unknown,
                                          int16_t target_temperature = 0,
                                          FanSetting fan = FanSetting::unknown) {
  BasicHvacState result{};
  result.power = power;
  result.mode = mode;
  result.target_temperature_centi_celsius = target_temperature;
  result.fan = fan;
  return result;
}

void run_device_reconciliation_tests() {
  {
    DeviceState state{};
    const CommandIntent intent = SetPowerIntent{PowerState::on};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::on), &state);
    assert(result.pending == PendingReconciliation::observed_converged);
    assert(result.cleared_desired_power);
    assert(!state.pending.intent.has_value());
    assert(!state.desired.power.has_value());
    assert(state.observed.has_value && state.observed.value.power == PowerState::on);
  }
  {
    DeviceState state{};
    const CommandIntent intent = SetModeIntent{HvacMode::cool};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::unknown, HvacMode::cool), &state);
    assert(result.pending == PendingReconciliation::observed_converged);
    assert(result.cleared_desired_mode);
    assert(!state.pending.intent.has_value() && !state.desired.mode.has_value());
    assert(state.observed.value.mode == HvacMode::cool);
  }
  {
    DeviceState state{};
    const CommandIntent intent = SetTargetTemperatureIntent{2050};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::unknown, HvacMode::unknown, 2050), &state);
    assert(result.pending == PendingReconciliation::observed_converged);
    assert(result.cleared_desired_target_temperature);
    assert(!state.pending.intent.has_value() && !state.desired.target_temperature_centi_celsius.has_value());
    assert(state.observed.value.target_temperature_centi_celsius == 2050);
  }
  {
    DeviceState state{};
    const CommandIntent intent = SetFanIntent{FanSetting::speed3};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::unknown, HvacMode::unknown, 0, FanSetting::speed3), &state);
    assert(result.pending == PendingReconciliation::observed_converged);
    assert(result.cleared_desired_fan);
    assert(!state.pending.intent.has_value() && !state.desired.fan.has_value());
    assert(state.observed.value.fan == FanSetting::speed3);
  }

  {
    DeviceState state{};
    const CommandIntent intent = SetPowerIntent{PowerState::on};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::off), &state);
    assert(result.pending == PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.power == PowerState::on);
    assert(state.observed.value.power == PowerState::off);
  }
  {
    DeviceState state{};
    const CommandIntent intent = SetModeIntent{HvacMode::cool};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::unknown, HvacMode::heat), &state)
               .pending == PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.mode == HvacMode::cool);
  }
  {
    DeviceState state{};
    const CommandIntent intent = SetTargetTemperatureIntent{2050};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::unknown, HvacMode::unknown, 2075), &state)
               .pending == PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.target_temperature_centi_celsius == 2050);
  }
  {
    DeviceState state{};
    const CommandIntent intent = SetFanIntent{FanSetting::speed3};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(
               reconciliation_observation(PowerState::unknown, HvacMode::unknown, 0, FanSetting::speed1), &state)
               .pending == PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.fan == FanSetting::speed3);
  }

  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::on}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetPowerIntent{PowerState::on}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::off}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::on), &state).pending ==
           PendingReconciliation::observed_converged);
    assert(!state.pending.intent.has_value() && state.desired.power == PowerState::off);
  }
  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::cool}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetModeIntent{HvacMode::cool}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::heat}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::unknown, HvacMode::cool), &state).pending ==
           PendingReconciliation::observed_converged);
    assert(!state.pending.intent.has_value() && state.desired.mode == HvacMode::heat);
  }
  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetTargetTemperatureIntent{2050}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetTargetTemperatureIntent{2050}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetTargetTemperatureIntent{2200}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::unknown, HvacMode::unknown, 2050), &state).pending ==
           PendingReconciliation::observed_converged);
    assert(!state.pending.intent.has_value() && state.desired.target_temperature_centi_celsius == 2200);
  }
  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed2}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetFanIntent{FanSetting::speed2}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed4}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(
               reconciliation_observation(PowerState::unknown, HvacMode::unknown, 0, FanSetting::speed2), &state)
               .pending == PendingReconciliation::observed_converged);
    assert(!state.pending.intent.has_value() && state.desired.fan == FanSetting::speed4);
  }

  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::on}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetPowerIntent{PowerState::on}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::off}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::off), &state).pending ==
           PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.power == PowerState::off);
  }
  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::cool}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetModeIntent{HvacMode::cool}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::heat}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::unknown, HvacMode::heat), &state).pending ==
           PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.mode == HvacMode::heat);
  }
  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetTargetTemperatureIntent{2050}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetTargetTemperatureIntent{2050}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetTargetTemperatureIntent{2200}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::unknown, HvacMode::unknown, 2200), &state).pending ==
           PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.target_temperature_centi_celsius == 2200);
  }
  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed2}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetFanIntent{FanSetting::speed2}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed4}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(
               reconciliation_observation(PowerState::unknown, HvacMode::unknown, 0, FanSetting::speed4), &state)
               .pending == PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.fan == FanSetting::speed4);
  }

  {
    DeviceState state{};
    const CommandIntent pending_power = SetPowerIntent{PowerState::on};
    assert(wfrac_device_model::stage_desired_intent(pending_power, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::cool}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(pending_power, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::off, HvacMode::cool), &state);
    assert(result.pending == PendingReconciliation::not_converged);
    assert(!result.cleared_desired_power && result.cleared_desired_mode);
    assert(state.pending.intent.has_value() && state.desired.power == PowerState::on);
    assert(!state.desired.mode.has_value());
  }

  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::on}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::heat}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetTargetTemperatureIntent{2050}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed3}, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::on, HvacMode::cool, 2050, FanSetting::speed2), &state);
    assert(result.pending == PendingReconciliation::none);
    assert(result.cleared_desired_power && result.cleared_desired_target_temperature);
    assert(!result.cleared_desired_mode && !result.cleared_desired_fan);
    assert(!state.desired.power.has_value() && !state.desired.target_temperature_centi_celsius.has_value());
    assert(state.desired.mode == HvacMode::heat && state.desired.fan == FanSetting::speed3);
  }

  {
    DeviceState state{};
    const CommandIntent pending_power = SetPowerIntent{PowerState::on};
    assert(wfrac_device_model::stage_desired_intent(pending_power, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(pending_power, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::unknown), &state);
    assert(result.pending == PendingReconciliation::not_converged);
    assert(state.pending.intent.has_value() && state.desired.power == PowerState::on);
  }

  {
    DeviceState state{};
    const CommandIntent pending_fan = SetFanIntent{FanSetting::speed2};
    assert(wfrac_device_model::begin_pending_command(pending_fan, &state) == DeviceStateError::none);
    const auto result = wfrac_device_model::reconcile_observation(
        reconciliation_observation(PowerState::unknown, HvacMode::unknown, 0, FanSetting::speed2), &state);
    assert(result.pending == PendingReconciliation::observed_converged);
    assert(!state.pending.intent.has_value());
    assert(!state.desired.fan.has_value());
  }

  {
    DeviceState state{};
    assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed2}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::reconcile_observation(
               reconciliation_observation(PowerState::unknown, HvacMode::unknown, 0, FanSetting::speed2), &state)
               .cleared_desired_fan);
    assert(!state.pending.intent.has_value() && !state.desired.fan.has_value());
  }

  {
    DeviceState state{};
    const CommandIntent intent = SetPowerIntent{PowerState::on};
    assert(wfrac_device_model::stage_desired_intent(intent, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(intent, &state) == DeviceStateError::none);
    assert(!state.observed.has_value);
    wfrac_device_model::apply_observation(reconciliation_observation(PowerState::on), &state);
    assert(state.observed.has_value && state.observed.value.power == PowerState::on);
    assert(state.pending.intent.has_value() && state.desired.power == PowerState::on);
  }

  {
    DeviceState state{};
    wfrac_device_model::set_availability(AvailabilityState::unavailable, &state);
    wfrac_device_model::reconcile_observation(reconciliation_observation(PowerState::on), &state);
    assert(state.availability == AvailabilityState::unavailable);
  }
}

}  // namespace device_reconciliation_tests
