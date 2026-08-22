#include <array>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

#include "wfrac_protocol/aircon_frame.hpp"
#include "wfrac_protocol/aircon_state.hpp"
#include "wfrac_protocol/capabilities.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {

std::vector<uint8_t> make_block(uint8_t segment_count, const std::vector<uint8_t>& segments, uint8_t state_five = 0) {
  assert(segments.size() == static_cast<std::size_t>(segment_count) * wfrac_protocol::kAirconSegmentBytes);
  std::vector<uint8_t> block(wfrac_protocol::kAirconStateBytes, 0);
  block[5] = state_five;
  block.push_back(segment_count);
  block.insert(block.end(), segments.begin(), segments.end());
  const uint16_t crc = wfrac_protocol::crc16_ccitt(block.data(), block.size());
  block.push_back(static_cast<uint8_t>(crc & 0xFFU));
  block.push_back(static_cast<uint8_t>(crc >> 8));
  return block;
}

std::vector<uint8_t> make_frame(uint8_t command_segments, uint8_t receive_segments) {
  const auto command = make_block(command_segments, std::vector<uint8_t>(command_segments * 4, 0xFF), 0xFF);
  const auto receive = make_block(receive_segments, std::vector<uint8_t>(receive_segments * 4, 0x00));
  std::vector<uint8_t> frame = command;
  frame.insert(frame.end(), receive.begin(), receive.end());
  return frame;
}

wfrac_protocol::AirconFrameView parse_ok(const std::vector<uint8_t>& raw) {
  wfrac_protocol::AirconFrameView frame{};
  assert(wfrac_protocol::parse_aircon_stat({raw.data(), raw.size()}, &frame) == wfrac_protocol::FrameError::none);
  return frame;
}

void assert_safe_baseline(const wfrac_protocol::CommandState& state, std::size_t changed_index) {
  assert(state[5] == 0xFFU);
  for (std::size_t index = 0; index < state.size(); ++index) {
    if (index != changed_index && index != 5) assert(state[index] == 0U);
  }
  assert(state[13] == 0U && state[14] == 0U && state[16] == 0U && state[17] == 0U);
}

wfrac_protocol::AirconBlockView receive_block(std::array<uint8_t, wfrac_protocol::kAirconStateBytes> state) {
  wfrac_protocol::AirconBlockView block{};
  block.state = state;
  return block;
}

}  // namespace

int main() {
  assert(wfrac_protocol::crc16_ccitt(nullptr, 0) == 0xFFFF);
  const std::array<uint8_t, 3> vector{{0x01, 0x02, 0x03}};
  assert(wfrac_protocol::crc16_ccitt(vector.data(), vector.size()) == 0xADAD);

  const auto no_op_block = make_block(1, {0xFF, 0xFF, 0xFF, 0xFF}, 0xFF);
  assert(no_op_block.size() == 25);
  auto valid = make_frame(1, 1);
  const auto parsed = parse_ok(valid);
  assert(parsed.command.state[5] == 0xFF);
  assert(parsed.command.segment_count == 1 && parsed.receive.segment_count == 1);
  assert(parsed.command.crc_valid && parsed.receive.crc_valid);

  const auto multiple = make_frame(2, 3);
  const auto multiple_parsed = parse_ok(multiple);
  assert(multiple_parsed.receive_offset == 18 + 1 + 2 * 4 + 2);
  assert(multiple_parsed.receive.segment_count == 3);

  const auto zero = make_frame(0, 0);
  assert(parse_ok(zero).receive_offset == 21);

  auto command_crc_bad = valid;
  command_crc_bad[0] ^= 0x01;
  const auto command_bad = parse_ok(command_crc_bad);
  assert(!command_bad.command.crc_valid && command_bad.receive.crc_valid);

  auto receive_crc_bad = valid;
  receive_crc_bad.back() ^= 0x01;
  const auto receive_bad = parse_ok(receive_crc_bad);
  assert(receive_bad.command.crc_valid && !receive_bad.receive.crc_valid);

  wfrac_protocol::AirconFrameView ignored{};
  assert(wfrac_protocol::parse_aircon_stat({valid.data(), 18}, &ignored) == wfrac_protocol::FrameError::too_short);
  assert(wfrac_protocol::parse_aircon_stat({valid.data(), 19}, &ignored) == wfrac_protocol::FrameError::truncated_segments);
  const auto truncated_receive = std::vector<uint8_t>(valid.begin(), valid.end() - 1);
  assert(wfrac_protocol::parse_aircon_stat({truncated_receive.data(), truncated_receive.size()}, &ignored) == wfrac_protocol::FrameError::missing_crc);
  const auto trailing = [&] { auto data = valid; data.push_back(0); return data; }();
  assert(wfrac_protocol::parse_aircon_stat({trailing.data(), trailing.size()}, &ignored) == wfrac_protocol::FrameError::invalid_layout);

  std::array<uint8_t, wfrac_protocol::kAirconStateBytes> receive_state{};
  receive_state[0] = 0x83U;
  receive_state[2] = 0x09U;
  receive_state[3] = 0x07U;
  receive_state[4] = 41U;
  receive_state[5] = 154U;
  auto semantic = wfrac_protocol::decode_receive_state(receive_block(receive_state));
  assert(semantic.model_nr_raw == 3U);
  assert(semantic.power == wfrac_protocol::PowerState::on);
  assert(semantic.operation_mode == wfrac_protocol::OperationMode::cool);
  assert(semantic.target_temperature_half_degrees == 41U);
  assert(semantic.fan == wfrac_protocol::FanMode::auto_mode);
  assert(semantic.controller_room_temperature_quarter_degrees == 93);

  receive_state[2] = 0x00U;
  assert(wfrac_protocol::decode_receive_state(receive_block(receive_state)).power == wfrac_protocol::PowerState::off);
  receive_state[2] = 0x03U;
  semantic = wfrac_protocol::decode_receive_state(receive_block(receive_state));
  assert(semantic.power == wfrac_protocol::PowerState::unknown && semantic.power_raw == 3U);
  for (const auto& pair : {std::pair<uint8_t, wfrac_protocol::OperationMode>{0x00U, wfrac_protocol::OperationMode::auto_mode},
                          {0x04U, wfrac_protocol::OperationMode::dry}, {0x08U, wfrac_protocol::OperationMode::cool},
                          {0x0CU, wfrac_protocol::OperationMode::fan}, {0x10U, wfrac_protocol::OperationMode::heat}}) {
    receive_state[2] = pair.first;
    assert(wfrac_protocol::decode_receive_state(receive_block(receive_state)).operation_mode == pair.second);
  }
  receive_state[2] = 0x1CU;
  assert(wfrac_protocol::decode_receive_state(receive_block(receive_state)).operation_mode == wfrac_protocol::OperationMode::unknown);
  for (const auto& pair : {std::pair<uint8_t, wfrac_protocol::FanMode>{0x07U, wfrac_protocol::FanMode::auto_mode},
                          {0x00U, wfrac_protocol::FanMode::speed1}, {0x01U, wfrac_protocol::FanMode::speed2},
                          {0x02U, wfrac_protocol::FanMode::speed3}, {0x06U, wfrac_protocol::FanMode::speed4}}) {
    receive_state[3] = pair.first;
    assert(wfrac_protocol::decode_receive_state(receive_block(receive_state)).fan == pair.second);
  }
  receive_state[3] = 0x05U;
  assert(wfrac_protocol::decode_receive_state(receive_block(receive_state)).fan == wfrac_protocol::FanMode::unknown);
  receive_state[4] = 40U;
  receive_state[5] = 154U;
  semantic = wfrac_protocol::decode_receive_state(receive_block(receive_state));
  assert(semantic.target_temperature_half_degrees == 40U);
  assert(semantic.controller_room_temperature_raw == 154U);
  assert(semantic.controller_room_temperature_quarter_degrees == 93);

  wfrac_protocol::CommandState command{};
  assert(wfrac_protocol::encode_set_power(wfrac_protocol::PowerState::off, &command) == wfrac_protocol::EncodeError::none);
  assert(command[2] == 0x02U); assert_safe_baseline(command, 2);
  assert(wfrac_protocol::encode_set_power(wfrac_protocol::PowerState::on, &command) == wfrac_protocol::EncodeError::none);
  assert(command[2] == 0x03U); assert_safe_baseline(command, 2);
  assert(wfrac_protocol::encode_set_power(wfrac_protocol::PowerState::unknown, &command) == wfrac_protocol::EncodeError::invalid_power);

  for (const auto& pair : {std::pair<wfrac_protocol::OperationMode, uint8_t>{wfrac_protocol::OperationMode::auto_mode, 0x20U},
                          {wfrac_protocol::OperationMode::dry, 0x24U}, {wfrac_protocol::OperationMode::cool, 0x28U},
                          {wfrac_protocol::OperationMode::fan, 0x2CU}, {wfrac_protocol::OperationMode::heat, 0x30U}}) {
    assert(wfrac_protocol::encode_set_mode(pair.first, &command) == wfrac_protocol::EncodeError::none);
    assert(command[2] == pair.second); assert_safe_baseline(command, 2);
  }
  assert(wfrac_protocol::encode_set_mode(wfrac_protocol::OperationMode::unknown, &command) == wfrac_protocol::EncodeError::invalid_mode);

  assert(wfrac_protocol::encode_set_target_temperature(40U, &command) == wfrac_protocol::EncodeError::none);
  assert(command[4] == 0xA8U); assert_safe_baseline(command, 4);
  assert(wfrac_protocol::encode_set_target_temperature(41U, &command) == wfrac_protocol::EncodeError::none);
  assert(command[4] == 0xA9U); assert_safe_baseline(command, 4);
  assert(wfrac_protocol::encode_set_target_temperature(0x80U, &command) == wfrac_protocol::EncodeError::invalid_target_temperature);

  for (const auto& pair : {std::pair<wfrac_protocol::FanMode, uint8_t>{wfrac_protocol::FanMode::auto_mode, 0x0FU},
                          {wfrac_protocol::FanMode::speed1, 0x08U}, {wfrac_protocol::FanMode::speed2, 0x09U},
                          {wfrac_protocol::FanMode::speed3, 0x0AU}, {wfrac_protocol::FanMode::speed4, 0x0EU}}) {
    assert(wfrac_protocol::encode_set_fan(pair.first, &command) == wfrac_protocol::EncodeError::none);
    assert(command[3] == pair.second); assert_safe_baseline(command, 3);
  }
  assert(wfrac_protocol::encode_set_fan(wfrac_protocol::FanMode::unknown, &command) == wfrac_protocol::EncodeError::invalid_fan);

  const auto assert_flags = [](const wfrac_protocol::CapabilityFlags& actual,
                               const wfrac_protocol::CapabilityFlags& expected) {
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
  };
  const auto assert_profile = [&](uint8_t raw, wfrac_protocol::CapabilityGroup group,
                                  const wfrac_protocol::CapabilityFlags& flags) {
    const auto profile = wfrac_protocol::decode_capabilities(raw);
    assert(profile.model_nr_raw == raw);
    assert(profile.profile_known);
    assert(profile.group == group);
    assert_flags(profile.flags, flags);
  };
  assert_profile(0U, wfrac_protocol::CapabilityGroup::separate_2021,
                 {true, false, false, true, true, true, true, false, true, true, false, false, false, true, true, false, false});
  assert_profile(1U, wfrac_protocol::CapabilityGroup::global_2022,
                 {true, true, true, true, true, false, true, true, true, true, false, false, true, false, true, false, false});
  assert_profile(2U, wfrac_protocol::CapabilityGroup::high_end_for_japanese_2023,
                 {true, false, true, true, true, false, false, false, true, true, false, true, false, false, true, false, false});
  assert_profile(3U, wfrac_protocol::CapabilityGroup::zt_2025,
                 {false, true, true, true, true, false, true, true, true, true, false, false, true, false, true, true, true});
  assert_profile(64U, wfrac_protocol::CapabilityGroup::fdt_2023,
                 {false, false, false, false, false, true, false, false, false, true, true, false, true, false, false, false, false});
  for (const uint8_t raw : {4U, 63U, 65U, 127U}) {
    const auto profile = wfrac_protocol::decode_capabilities(raw);
    assert(profile.model_nr_raw == raw);
    assert(!profile.profile_known);
    assert(profile.group == wfrac_protocol::CapabilityGroup::unknown);
    assert_flags(profile.flags, {});
  }
  return 0;
}
