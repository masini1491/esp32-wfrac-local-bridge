#pragma once

#include <array>
#include <cassert>
#include <cstdint>

#include "wfrac_device_model/protocol_command_mapping.hpp"

namespace device_command_mapping_tests {

using wfrac_device_model::AvailabilityState;
using wfrac_device_model::BasicHvacState;
using wfrac_device_model::CommandIntent;
using wfrac_device_model::DeviceState;
using wfrac_device_model::DeviceStateError;
using wfrac_device_model::FanSetting;
using wfrac_device_model::HvacMode;
using wfrac_device_model::PowerState;
using wfrac_device_model::ProtocolCommandMappingError;
using wfrac_device_model::SetFanIntent;
using wfrac_device_model::SetModeIntent;
using wfrac_device_model::SetPowerIntent;
using wfrac_device_model::SetTargetTemperatureIntent;

void assert_safe_command_invariants(const wfrac_protocol::CommandState& state) {
  assert(state[5] == 0xFFU);
  assert(state[13] == 0U && state[14] == 0U && state[16] == 0U && state[17] == 0U);
}

void assert_only_command_byte_is_set(const wfrac_protocol::CommandState& state, std::size_t command_index) {
  assert_safe_command_invariants(state);
  for (std::size_t index = 0U; index < state.size(); ++index) {
    if (index != command_index && index != 5U) assert(state[index] == 0U);
  }
}

void run_device_command_mapping_tests() {
  {
    for (const auto& item : std::array<std::pair<PowerState, uint8_t>, 2>{{
             {PowerState::off, 0x02U}, {PowerState::on, 0x03U}}}) {
      wfrac_protocol::CommandState mapped{};
      const CommandIntent intent = SetPowerIntent{item.first};
      assert(wfrac_device_model::map_command_intent_to_protocol(intent, &mapped) == ProtocolCommandMappingError::none);
      assert(mapped[2] == item.second);
      assert_only_command_byte_is_set(mapped, 2U);
      wfrac_protocol::CommandState direct{};
      assert(wfrac_protocol::encode_set_power(
                 item.first == PowerState::off ? wfrac_protocol::PowerState::off : wfrac_protocol::PowerState::on,
                 &direct) == wfrac_protocol::EncodeError::none);
      assert(mapped == direct);
    }
  }

  {
    for (const auto& item : std::array<std::pair<HvacMode, uint8_t>, 5>{{
             {HvacMode::auto_mode, 0x20U}, {HvacMode::cool, 0x28U}, {HvacMode::heat, 0x30U},
             {HvacMode::fan, 0x2CU}, {HvacMode::dry, 0x24U}}}) {
      wfrac_protocol::CommandState mapped{};
      const CommandIntent intent = SetModeIntent{item.first};
      assert(wfrac_device_model::map_command_intent_to_protocol(intent, &mapped) == ProtocolCommandMappingError::none);
      assert(mapped[2] == item.second);
      assert_only_command_byte_is_set(mapped, 2U);
      wfrac_protocol::OperationMode protocol_mode{};
      switch (item.first) {
        case HvacMode::auto_mode: protocol_mode = wfrac_protocol::OperationMode::auto_mode; break;
        case HvacMode::cool: protocol_mode = wfrac_protocol::OperationMode::cool; break;
        case HvacMode::heat: protocol_mode = wfrac_protocol::OperationMode::heat; break;
        case HvacMode::fan: protocol_mode = wfrac_protocol::OperationMode::fan; break;
        default: protocol_mode = wfrac_protocol::OperationMode::dry; break;
      }
      wfrac_protocol::CommandState direct{};
      assert(wfrac_protocol::encode_set_mode(protocol_mode, &direct) == wfrac_protocol::EncodeError::none);
      assert(mapped == direct);
    }
  }

  {
    for (const auto& item : std::array<std::pair<FanSetting, uint8_t>, 5>{{
             {FanSetting::auto_mode, 0x0FU}, {FanSetting::speed1, 0x08U}, {FanSetting::speed2, 0x09U},
             {FanSetting::speed3, 0x0AU}, {FanSetting::speed4, 0x0EU}}}) {
      wfrac_protocol::CommandState mapped{};
      const CommandIntent intent = SetFanIntent{item.first};
      assert(wfrac_device_model::map_command_intent_to_protocol(intent, &mapped) == ProtocolCommandMappingError::none);
      assert(mapped[3] == item.second);
      assert_only_command_byte_is_set(mapped, 3U);
      wfrac_protocol::FanMode protocol_fan{};
      switch (item.first) {
        case FanSetting::auto_mode: protocol_fan = wfrac_protocol::FanMode::auto_mode; break;
        case FanSetting::speed1: protocol_fan = wfrac_protocol::FanMode::speed1; break;
        case FanSetting::speed2: protocol_fan = wfrac_protocol::FanMode::speed2; break;
        case FanSetting::speed3: protocol_fan = wfrac_protocol::FanMode::speed3; break;
        default: protocol_fan = wfrac_protocol::FanMode::speed4; break;
      }
      wfrac_protocol::CommandState direct{};
      assert(wfrac_protocol::encode_set_fan(protocol_fan, &direct) == wfrac_protocol::EncodeError::none);
      assert(mapped == direct);
    }
  }

  {
    for (const auto& item : std::array<std::pair<int16_t, uint8_t>, 5>{{
             {0, 0x80U}, {50, 0x81U}, {2000, 0xA8U}, {2050, 0xA9U}, {6350, 0xFFU}}}) {
      wfrac_protocol::CommandState mapped{};
      const CommandIntent intent = SetTargetTemperatureIntent{item.first};
      assert(wfrac_device_model::map_command_intent_to_protocol(intent, &mapped) == ProtocolCommandMappingError::none);
      assert(mapped[4] == item.second);
      assert_only_command_byte_is_set(mapped, 4U);
      wfrac_protocol::CommandState direct{};
      assert(wfrac_protocol::encode_set_target_temperature(static_cast<uint8_t>(item.first / 50), &direct) ==
             wfrac_protocol::EncodeError::none);
      assert(mapped == direct);
    }
  }

  for (const int16_t invalid_temperature : {-50, 25, 2025, 2051, 2075, 6400}) {
    wfrac_protocol::CommandState mapped{};
    mapped.fill(0xAAU);
    assert(wfrac_device_model::map_command_intent_to_protocol(SetTargetTemperatureIntent{invalid_temperature}, &mapped) ==
           ProtocolCommandMappingError::target_temperature_not_representable);
    assert(mapped == wfrac_protocol::make_safe_command_state());
  }

  for (const CommandIntent invalid_intent : {CommandIntent{SetPowerIntent{PowerState::unknown}},
                                             CommandIntent{SetModeIntent{HvacMode::unknown}},
                                             CommandIntent{SetFanIntent{FanSetting::unknown}}}) {
    wfrac_protocol::CommandState mapped{};
    mapped.fill(0xAAU);
    assert(wfrac_device_model::map_command_intent_to_protocol(invalid_intent, &mapped) ==
           ProtocolCommandMappingError::invalid_control_target);
    assert(mapped == wfrac_protocol::make_safe_command_state());
  }

  assert(wfrac_device_model::map_command_intent_to_protocol(SetPowerIntent{PowerState::on}, nullptr) ==
         ProtocolCommandMappingError::invalid_argument);

  {
    DeviceState state{};
    BasicHvacState observed{};
    observed.power = PowerState::off;
    observed.mode = HvacMode::heat;
    observed.target_temperature_centi_celsius = 2150;
    observed.fan = FanSetting::speed1;
    wfrac_device_model::apply_observation(observed, &state);
    assert(wfrac_device_model::stage_desired_intent(SetModeIntent{HvacMode::cool}, &state) == DeviceStateError::none);
    assert(wfrac_device_model::begin_pending_command(SetFanIntent{FanSetting::speed4}, &state) == DeviceStateError::none);
    wfrac_device_model::set_availability(AvailabilityState::unavailable, &state);

    wfrac_protocol::CommandState mapped{};
    assert(wfrac_device_model::map_command_intent_to_protocol(SetPowerIntent{PowerState::on}, &mapped) ==
           ProtocolCommandMappingError::none);
    assert(state.observed.has_value && state.observed.value.power == PowerState::off &&
           state.observed.value.mode == HvacMode::heat && state.observed.value.target_temperature_centi_celsius == 2150 &&
           state.observed.value.fan == FanSetting::speed1);
    assert(state.desired.mode == HvacMode::cool);
    assert(state.pending.intent.has_value() && std::get<SetFanIntent>(*state.pending.intent).value == FanSetting::speed4);
    assert(state.availability == AvailabilityState::unavailable);
  }
}

}  // namespace device_command_mapping_tests
