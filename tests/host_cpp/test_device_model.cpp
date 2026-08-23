#include <array>
#include <cassert>
#include <cstdint>
#include <utility>

#include "device_command_mapping_test_cases.hpp"
#include "device_operation_query_request_test_cases.hpp"
#include "device_request_mapping_test_cases.hpp"
#include "device_reconciliation_test_cases.hpp"
#include "device_state_test_cases.hpp"
#include "wfrac_device_model/protocol_mapping.hpp"
#include "wfrac_device_model/protocol_observation.hpp"

namespace {

void assert_hints(const wfrac_device_model::CapabilityHints& actual,
                  const wfrac_device_model::CapabilityHints& expected) {
  assert(actual.power_consumption == expected.power_consumption);
  assert(actual.vacant_property == expected.vacant_property);
  assert(actual.self_clean_operation == expected.self_clean_operation);
  assert(actual.wind_direction_lr == expected.wind_direction_lr);
  assert(actual.entrust == expected.entrust);
  assert(actual.led_light == expected.led_light);
  assert(actual.auto_heating == expected.auto_heating);
  assert(actual.home_leave_mode == expected.home_leave_mode);
  assert(actual.temp_information == expected.temp_information);
  assert(actual.indoor_and_outdoor_temp == expected.indoor_and_outdoor_temp);
  assert(actual.preset_temp_auto == expected.preset_temp_auto);
  assert(actual.call_center_phone_number == expected.call_center_phone_number);
  assert(actual.outdoor_temp_always_show == expected.outdoor_temp_always_show);
  assert(actual.outdoor_temp_outside_tokyo == expected.outdoor_temp_outside_tokyo);
  assert(actual.cool_hot_judge == expected.cool_hot_judge);
  assert(actual.preset_temp_range_2 == expected.preset_temp_range_2);
  assert(actual.operation_data == expected.operation_data);
}

wfrac_protocol::AirconFrameView observation_frame(uint8_t receive_byte2, uint8_t receive_byte3,
                                                   uint8_t target, uint8_t controller_temperature) {
  wfrac_protocol::AirconFrameView frame{};
  frame.command.encoded_size = 21U;
  frame.command.crc_valid = true;
  frame.receive.encoded_size = 21U;
  frame.receive.crc_valid = true;
  frame.receive.state[2] = receive_byte2;
  frame.receive.state[3] = receive_byte3;
  frame.receive.state[4] = target;
  frame.receive.state[5] = controller_temperature;
  frame.receive_offset = 21U;
  return frame;
}

}  // namespace

int main() {
  wfrac_protocol::ReceiveState protocol_state{};
  protocol_state.target_temperature_half_degrees = 40U;
  protocol_state.controller_room_temperature_quarter_degrees = 93;
  for (const auto& pair : {std::pair<wfrac_protocol::PowerState, wfrac_device_model::PowerState>{
                                wfrac_protocol::PowerState::off, wfrac_device_model::PowerState::off},
                            {wfrac_protocol::PowerState::on, wfrac_device_model::PowerState::on},
                            {wfrac_protocol::PowerState::unknown, wfrac_device_model::PowerState::unknown}}) {
    protocol_state.power = pair.first;
    assert(wfrac_device_model::map_basic_hvac_state(protocol_state).power == pair.second);
  }
  for (const auto& pair : {std::pair<wfrac_protocol::OperationMode, wfrac_device_model::HvacMode>{
                                wfrac_protocol::OperationMode::auto_mode, wfrac_device_model::HvacMode::auto_mode},
                            {wfrac_protocol::OperationMode::cool, wfrac_device_model::HvacMode::cool},
                            {wfrac_protocol::OperationMode::heat, wfrac_device_model::HvacMode::heat},
                            {wfrac_protocol::OperationMode::fan, wfrac_device_model::HvacMode::fan},
                            {wfrac_protocol::OperationMode::dry, wfrac_device_model::HvacMode::dry},
                            {wfrac_protocol::OperationMode::unknown, wfrac_device_model::HvacMode::unknown}}) {
    protocol_state.operation_mode = pair.first;
    assert(wfrac_device_model::map_basic_hvac_state(protocol_state).mode == pair.second);
  }
  for (const auto& pair : {std::pair<wfrac_protocol::FanMode, wfrac_device_model::FanSetting>{
                                wfrac_protocol::FanMode::auto_mode, wfrac_device_model::FanSetting::auto_mode},
                            {wfrac_protocol::FanMode::speed1, wfrac_device_model::FanSetting::speed1},
                            {wfrac_protocol::FanMode::speed2, wfrac_device_model::FanSetting::speed2},
                            {wfrac_protocol::FanMode::speed3, wfrac_device_model::FanSetting::speed3},
                            {wfrac_protocol::FanMode::speed4, wfrac_device_model::FanSetting::speed4},
                            {wfrac_protocol::FanMode::unknown, wfrac_device_model::FanSetting::unknown}}) {
    protocol_state.fan = pair.first;
    assert(wfrac_device_model::map_basic_hvac_state(protocol_state).fan == pair.second);
  }
  auto state = wfrac_device_model::map_basic_hvac_state(protocol_state);
  assert(state.target_temperature_centi_celsius == 2000);
  assert(state.controller_room_temperature_centi_celsius == 2325);
  protocol_state.target_temperature_half_degrees = 41U;
  protocol_state.controller_room_temperature_quarter_degrees = -4;
  state = wfrac_device_model::map_basic_hvac_state(protocol_state);
  assert(state.target_temperature_centi_celsius == 2050);
  assert(state.controller_room_temperature_centi_celsius == -100);
  protocol_state.target_temperature_half_degrees = 255U;
  assert(wfrac_device_model::map_basic_hvac_state(protocol_state).target_temperature_centi_celsius == 12750);

  const auto valid_frame = observation_frame(0x09U, 0x02U, 47U, 154U);
  wfrac_device_model::BasicHvacState observed{};
  assert(wfrac_device_model::map_aircon_frame_observation(valid_frame, &observed) ==
         wfrac_device_model::ObservationMappingError::none);
  assert(observed.power == wfrac_device_model::PowerState::on &&
         observed.mode == wfrac_device_model::HvacMode::cool &&
         observed.fan == wfrac_device_model::FanSetting::speed3 &&
         observed.target_temperature_centi_celsius == 2350 &&
         observed.controller_room_temperature_centi_celsius == 2325);
  for (const auto& pair : {std::pair<uint8_t, wfrac_device_model::HvacMode>{0x01U, wfrac_device_model::HvacMode::auto_mode},
                           {0x09U, wfrac_device_model::HvacMode::cool},
                           {0x11U, wfrac_device_model::HvacMode::heat}}) {
    assert(wfrac_device_model::map_aircon_frame_observation(observation_frame(pair.first, 0x02U, 47U, 154U), &observed) ==
           wfrac_device_model::ObservationMappingError::none);
    assert(observed.mode == pair.second);
  }
  assert(wfrac_device_model::map_aircon_frame_observation(observation_frame(0x03U, 0x02U, 47U, 154U), &observed) ==
         wfrac_device_model::ObservationMappingError::none);
  assert(observed.power == wfrac_device_model::PowerState::unknown);
  assert(wfrac_device_model::map_aircon_frame_observation(observation_frame(0x01U, 0x05U, 47U, 154U), &observed) ==
         wfrac_device_model::ObservationMappingError::none);
  assert(observed.fan == wfrac_device_model::FanSetting::unknown);
  assert(wfrac_device_model::map_aircon_frame_observation(observation_frame(0x1DU, 0x02U, 47U, 154U), &observed) ==
         wfrac_device_model::ObservationMappingError::none);
  assert(observed.mode == wfrac_device_model::HvacMode::unknown);

  auto command_crc_bad = valid_frame;
  command_crc_bad.command.crc_valid = false;
  observed.power = wfrac_device_model::PowerState::on;
  assert(wfrac_device_model::map_aircon_frame_observation(command_crc_bad, &observed) ==
         wfrac_device_model::ObservationMappingError::crc_mismatch);
  assert(observed.power == wfrac_device_model::PowerState::unknown);
  auto receive_crc_bad = valid_frame;
  receive_crc_bad.receive.crc_valid = false;
  assert(wfrac_device_model::map_aircon_frame_observation(receive_crc_bad, &observed) ==
         wfrac_device_model::ObservationMappingError::crc_mismatch);
  auto invalid_frame = valid_frame;
  invalid_frame.receive.encoded_size = 0U;
  assert(wfrac_device_model::map_aircon_frame_observation(invalid_frame, &observed) ==
         wfrac_device_model::ObservationMappingError::invalid_frame);

  wfrac_device_model::DeviceState observation_state{};
  assert(wfrac_device_model::stage_desired_intent(wfrac_device_model::SetModeIntent{wfrac_device_model::HvacMode::heat},
                                                  &observation_state) == wfrac_device_model::DeviceStateError::none);
  assert(wfrac_device_model::begin_pending_command(wfrac_device_model::SetPowerIntent{wfrac_device_model::PowerState::on},
                                                    &observation_state) == wfrac_device_model::DeviceStateError::none);
  assert(wfrac_device_model::apply_aircon_frame_observation(valid_frame, &observation_state) ==
         wfrac_device_model::ObservationMappingError::none);
  assert(observation_state.observed.has_value && observation_state.observed.value.power == wfrac_device_model::PowerState::on &&
         observation_state.desired.mode.has_value() && observation_state.pending.intent.has_value());
  const auto before_failure = observation_state;
  assert(wfrac_device_model::apply_aircon_frame_observation(receive_crc_bad, &observation_state) ==
         wfrac_device_model::ObservationMappingError::crc_mismatch);
  assert(observation_state.observed.has_value == before_failure.observed.has_value &&
         observation_state.observed.value.power == before_failure.observed.value.power &&
         observation_state.desired.mode == before_failure.desired.mode &&
         observation_state.pending.intent.has_value() == before_failure.pending.intent.has_value());
  const auto reconciled = wfrac_device_model::reconcile_aircon_frame_observation(valid_frame, &observation_state);
  assert(reconciled.error == wfrac_device_model::ObservationMappingError::none &&
         reconciled.reconciliation.pending == wfrac_device_model::PendingReconciliation::observed_converged &&
         !observation_state.pending.intent.has_value());
  assert(wfrac_device_model::stage_desired_intent(wfrac_device_model::SetModeIntent{wfrac_device_model::HvacMode::cool},
                                                  &observation_state) == wfrac_device_model::DeviceStateError::none);
  const auto desired_reconciled = wfrac_device_model::reconcile_aircon_frame_observation(valid_frame, &observation_state);
  assert(desired_reconciled.reconciliation.cleared_desired_mode && !observation_state.desired.mode.has_value());
  assert(wfrac_device_model::stage_desired_intent(wfrac_device_model::SetPowerIntent{wfrac_device_model::PowerState::off},
                                                  &observation_state) == wfrac_device_model::DeviceStateError::none);
  assert(wfrac_device_model::begin_pending_command(wfrac_device_model::SetPowerIntent{wfrac_device_model::PowerState::on},
                                                    &observation_state) == wfrac_device_model::DeviceStateError::none);
  const auto protected_reconciled = wfrac_device_model::reconcile_aircon_frame_observation(valid_frame, &observation_state);
  assert(protected_reconciled.reconciliation.pending == wfrac_device_model::PendingReconciliation::observed_converged &&
         observation_state.desired.power == wfrac_device_model::PowerState::off);

  const auto assert_profile = [](uint8_t raw, const wfrac_device_model::CapabilityHints& expected) {
    const auto profile = wfrac_protocol::decode_capabilities(raw);
    assert(profile.profile_known);
    assert_hints(wfrac_device_model::map_capability_hints(profile), expected);
  };
  using Hint = wfrac_device_model::CapabilityHint;
  assert_profile(0U, {Hint::advertised_true, Hint::advertised_false, Hint::advertised_false, Hint::advertised_true,
                      Hint::advertised_true, Hint::advertised_true, Hint::advertised_true, Hint::advertised_false,
                      Hint::advertised_true, Hint::advertised_true, Hint::advertised_false, Hint::advertised_false,
                      Hint::advertised_false, Hint::advertised_true, Hint::advertised_true, Hint::advertised_false,
                      Hint::advertised_false});
  assert_profile(3U, {Hint::advertised_false, Hint::advertised_true, Hint::advertised_true, Hint::advertised_true,
                      Hint::advertised_true, Hint::advertised_false, Hint::advertised_true, Hint::advertised_true,
                      Hint::advertised_true, Hint::advertised_true, Hint::advertised_false, Hint::advertised_false,
                      Hint::advertised_true, Hint::advertised_false, Hint::advertised_true, Hint::advertised_true,
                      Hint::advertised_true});
  assert_profile(64U, {Hint::advertised_false, Hint::advertised_false, Hint::advertised_false, Hint::advertised_false,
                       Hint::advertised_false, Hint::advertised_true, Hint::advertised_false, Hint::advertised_false,
                       Hint::advertised_false, Hint::advertised_true, Hint::advertised_true, Hint::advertised_false,
                       Hint::advertised_true, Hint::advertised_false, Hint::advertised_false, Hint::advertised_false,
                       Hint::advertised_false});
  const auto unknown_profile = wfrac_protocol::decode_capabilities(127U);
  assert(!unknown_profile.profile_known);
  assert_hints(wfrac_device_model::map_capability_hints(unknown_profile), {});
  run_device_state_ownership_tests();
  device_reconciliation_tests::run_device_reconciliation_tests();
  device_command_mapping_tests::run_device_command_mapping_tests();
  device_request_mapping_tests::run_device_request_mapping_tests();
  device_operation_query_request_tests::run_device_operation_query_request_tests();
  return 0;
}
