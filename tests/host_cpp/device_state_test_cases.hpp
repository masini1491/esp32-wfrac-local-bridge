#pragma once

#include <cassert>

#include "wfrac_device_model/device_state.hpp"

namespace {

using wfrac_device_model::AvailabilityState;
using wfrac_device_model::BasicHvacState;
using wfrac_device_model::CommandIntent;
using wfrac_device_model::DeviceState;
using wfrac_device_model::DeviceStateError;
using wfrac_device_model::FanSetting;
using wfrac_device_model::HvacMode;
using wfrac_device_model::PendingCommandOutcome;
using wfrac_device_model::PowerState;
using wfrac_device_model::SetFanIntent;
using wfrac_device_model::SetModeIntent;
using wfrac_device_model::SetPowerIntent;
using wfrac_device_model::SetTargetTemperatureIntent;

void assert_desired_empty(const DeviceState& state) {
  assert(!state.desired.power.has_value());
  assert(!state.desired.mode.has_value());
  assert(!state.desired.target_temperature_centi_celsius.has_value());
  assert(!state.desired.fan.has_value());
}

void run_device_state_ownership_tests() {
  DeviceState initial{};
  assert(!initial.observed.has_value);
  assert(initial.observed.value.target_temperature_centi_celsius == 0);
  assert_desired_empty(initial);
  assert(!initial.pending.intent.has_value());
  assert(initial.availability == AvailabilityState::unknown);

  BasicHvacState zero_observation{};
  zero_observation.power = PowerState::off;
  zero_observation.mode = HvacMode::auto_mode;
  zero_observation.fan = FanSetting::auto_mode;
  wfrac_device_model::apply_observation(zero_observation, &initial);
  assert(initial.observed.has_value);
  assert(initial.observed.value.target_temperature_centi_celsius == 0);
  assert(initial.observed.value.controller_room_temperature_centi_celsius == 0);

  DeviceState desired{};
  assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::on}, &desired) == DeviceStateError::none);
  assert(desired.desired.power == PowerState::on);
  assert(!desired.desired.mode.has_value());
  assert(!desired.desired.target_temperature_centi_celsius.has_value());
  assert(!desired.desired.fan.has_value());
  assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::off}, &desired) == DeviceStateError::none);
  assert(desired.desired.power == PowerState::off);
  assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::heat}, &desired) == DeviceStateError::none);
  assert(wfrac_device_model::stage_desired_intent(SetTargetTemperatureIntent{2050}, &desired) == DeviceStateError::none);
  assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::speed3}, &desired) == DeviceStateError::none);
  assert(desired.desired.mode == HvacMode::heat);
  assert(desired.desired.target_temperature_centi_celsius == 2050);
  assert(desired.desired.fan == FanSetting::speed3);

  assert(wfrac_device_model::stage_desired_intent(SetPowerIntent{PowerState::unknown}, &desired) ==
         DeviceStateError::invalid_control_target);
  assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::unknown}, &desired) ==
         DeviceStateError::invalid_control_target);
  assert(wfrac_device_model::stage_desired_intent(SetFanIntent{FanSetting::unknown}, &desired) ==
         DeviceStateError::invalid_control_target);

  DeviceState invalid_pending{};
  assert(wfrac_device_model::begin_pending_command(SetPowerIntent{PowerState::unknown}, &invalid_pending) ==
         DeviceStateError::invalid_control_target);
  assert(!invalid_pending.pending.intent.has_value());
  assert(wfrac_device_model::begin_pending_command(SetModeIntent{HvacMode::unknown}, &invalid_pending) ==
         DeviceStateError::invalid_control_target);
  assert(!invalid_pending.pending.intent.has_value());
  assert(wfrac_device_model::begin_pending_command(SetFanIntent{FanSetting::unknown}, &invalid_pending) ==
         DeviceStateError::invalid_control_target);
  assert(!invalid_pending.pending.intent.has_value());

  DeviceState ownership{};
  wfrac_device_model::apply_observation(zero_observation, &ownership);
  const CommandIntent turn_on = SetPowerIntent{PowerState::on};
  assert(wfrac_device_model::stage_desired_intent(turn_on, &ownership) == DeviceStateError::none);
  assert(wfrac_device_model::begin_pending_command(turn_on, &ownership) == DeviceStateError::none);
  assert(ownership.observed.value.power == PowerState::off);
  assert(wfrac_device_model::finish_pending_command(PendingCommandOutcome::confirmed, &ownership) == DeviceStateError::none);
  assert(ownership.observed.value.power == PowerState::off);
  BasicHvacState on_observation = zero_observation;
  on_observation.power = PowerState::on;
  wfrac_device_model::apply_observation(on_observation, &ownership);
  assert(ownership.observed.value.power == PowerState::on);
  assert(ownership.desired.power == PowerState::on);

  DeviceState pending{};
  const CommandIntent set_cool = SetModeIntent{HvacMode::cool};
  const CommandIntent set_fan = SetFanIntent{FanSetting::speed2};
  assert(wfrac_device_model::begin_pending_command(set_cool, &pending) == DeviceStateError::none);
  assert(wfrac_device_model::begin_pending_command(set_fan, &pending) == DeviceStateError::already_pending);
  assert(wfrac_device_model::finish_pending_command(PendingCommandOutcome::rejected, &pending) == DeviceStateError::none);
  assert(!pending.pending.intent.has_value());
  assert(wfrac_device_model::begin_pending_command(set_fan, &pending) == DeviceStateError::none);

  for (const auto outcome : {PendingCommandOutcome::confirmed, PendingCommandOutcome::rejected,
                             PendingCommandOutcome::failed, PendingCommandOutcome::timed_out}) {
    DeviceState resolved{};
    wfrac_device_model::apply_observation(zero_observation, &resolved);
    assert(wfrac_device_model::stage_desired_intent(turn_on, &resolved) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(turn_on, &resolved) == DeviceStateError::none);
    assert(wfrac_device_model::finish_pending_command(outcome, &resolved) == DeviceStateError::none);
    assert(!resolved.pending.intent.has_value());
    assert(resolved.observed.value.power == PowerState::off);
    assert(resolved.desired.power == PowerState::on);
  }

  wfrac_device_model::set_availability(AvailabilityState::available, &ownership);
  wfrac_device_model::set_availability(AvailabilityState::unavailable, &ownership);
  assert(ownership.availability == AvailabilityState::unavailable);
  assert(ownership.observed.has_value);
  assert(ownership.desired.power == PowerState::on);

  wfrac_device_model::clear_all_desired(&ownership);
  assert_desired_empty(ownership);
}

}  // namespace
