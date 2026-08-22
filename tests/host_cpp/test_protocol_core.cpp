#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "wfrac_protocol/aircon_stat.hpp"
#include "wfrac_protocol/base64.hpp"
#include "wfrac_protocol/aircon_frame.hpp"
#include "wfrac_protocol/aircon_block.hpp"
#include "wfrac_protocol/aircon_state.hpp"
#include "wfrac_protocol/capabilities.hpp"
#include "wfrac_protocol/crc16.hpp"
#include "wfrac_protocol/operation_data.hpp"

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

  const auto assert_operation = [](const wfrac_protocol::OperationSegment& raw,
                                   wfrac_protocol::OperationSemanticKind kind,
                                   wfrac_protocol::OperationScope scope,
                                   wfrac_protocol::OperationValueQuality quality,
                                   bool code_known = true, bool unit_relation_known = false) {
    const auto decoded = wfrac_protocol::decode_operation_segment(raw);
    assert(decoded.raw.code == raw.code && decoded.raw.op1 == raw.op1);
    assert(decoded.raw.op2 == raw.op2 && decoded.raw.op3 == raw.op3);
    assert(decoded.kind == kind);
    assert(decoded.scope == scope);
    assert(decoded.value_quality == quality);
    assert(decoded.code_known == code_known);
    assert(decoded.unit_relation_known == unit_relation_known);
  };
  assert_operation({0x80U, 0x10U, 0xABU, 0xCDU}, wfrac_protocol::OperationSemanticKind::outdoor_ambient,
                    wfrac_protocol::OperationScope::shared_outdoor,
                    wfrac_protocol::OperationValueQuality::canonical_ambient_raw_pending_lookup);
  assert_operation({0x80U, 0x20U, 0x01U, 0xFEU}, wfrac_protocol::OperationSemanticKind::indoor_ambient,
                    wfrac_protocol::OperationScope::per_indoor,
                    wfrac_protocol::OperationValueQuality::canonical_ambient_raw_pending_lookup);
  assert_operation({0x80U, 0x30U, 0x02U, 0x03U}, wfrac_protocol::OperationSemanticKind::unknown,
                    wfrac_protocol::OperationScope::unknown,
                    wfrac_protocol::OperationValueQuality::unknown_semantic, false);
  assert_operation({0x94U, 0x10U, 0x12U, 0x34U}, wfrac_protocol::OperationSemanticKind::current_run_energy,
                    wfrac_protocol::OperationScope::per_indoor,
                    wfrac_protocol::OperationValueQuality::bounded_derived_available_later, true, true);
  assert_operation({0x11U, 0x01U, 0x02U, 0x03U}, wfrac_protocol::OperationSemanticKind::compressor_frequency,
                    wfrac_protocol::OperationScope::shared_outdoor, wfrac_protocol::OperationValueQuality::raw_only);
  assert_operation({0x90U, 0x04U, 0x05U, 0x06U}, wfrac_protocol::OperationSemanticKind::operating_current,
                    wfrac_protocol::OperationScope::shared_outdoor, wfrac_protocol::OperationValueQuality::raw_only);
  assert_operation({0x85U, 0x07U, 0x08U, 0x09U}, wfrac_protocol::OperationSemanticKind::discharge_hot_gas_temperature,
                    wfrac_protocol::OperationScope::shared_outdoor,
                    wfrac_protocol::OperationValueQuality::bounded_derived_available_later);
  assert_operation({0x13U, 0x0AU, 0x0BU, 0x0CU}, wfrac_protocol::OperationSemanticKind::eev_raw,
                    wfrac_protocol::OperationScope::per_indoor, wfrac_protocol::OperationValueQuality::raw_only);
  assert_operation({0x81U, 0x10U, 171U, 0x0DU}, wfrac_protocol::OperationSemanticKind::thi_r1,
                    wfrac_protocol::OperationScope::per_indoor,
                    wfrac_protocol::OperationValueQuality::bounded_derived_available_later);
  assert_operation({0x87U, 0x11U, 171U, 0x0EU}, wfrac_protocol::OperationSemanticKind::thi_r3,
                    wfrac_protocol::OperationScope::per_indoor,
                    wfrac_protocol::OperationValueQuality::bounded_derived_available_later);
  assert_operation({0x82U, 0x12U, 0x0FU, 0x10U}, wfrac_protocol::OperationSemanticKind::tho_r1,
                    wfrac_protocol::OperationScope::shared_outdoor, wfrac_protocol::OperationValueQuality::raw_only);
  assert_operation({0xB1U, 0x13U, 0x11U, 0x12U}, wfrac_protocol::OperationSemanticKind::tdsh_raw,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::raw_only);
  assert_operation({0x7CU, 0x14U, 0x13U, 0x14U}, wfrac_protocol::OperationSemanticKind::protection_diagnostic_unknown,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::unknown_semantic);
  assert_operation({0x1EU, 0x15U, 0x16U, 0x17U}, wfrac_protocol::OperationSemanticKind::run_hours_candidate,
                    wfrac_protocol::OperationScope::unknown,
                    wfrac_protocol::OperationValueQuality::no_usable_value_established);
  assert_operation({0x1FU, 0x18U, 0x19U, 0x1AU}, wfrac_protocol::OperationSemanticKind::fan_speed_candidate,
                    wfrac_protocol::OperationScope::unknown,
                    wfrac_protocol::OperationValueQuality::no_usable_value_established);
  assert_operation({0x0DU, 0x19U, 0x1AU, 0x1BU}, wfrac_protocol::OperationSemanticKind::unknown_semantic_0d,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::unknown_semantic);
  assert_operation({0x21U, 0x1CU, 0x1DU, 0x1EU}, wfrac_protocol::OperationSemanticKind::unknown_semantic_21,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::unknown_semantic);
  assert_operation({0x32U, 0x1FU, 0x20U, 0x21U}, wfrac_protocol::OperationSemanticKind::unknown_semantic_32,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::unknown_semantic);
  assert_operation({0x34U, 0x22U, 0x23U, 0xA5U}, wfrac_protocol::OperationSemanticKind::unknown_semantic_34,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::unknown_semantic);
  assert_operation({0x35U, 0x24U, 0x25U, 0x26U}, wfrac_protocol::OperationSemanticKind::unknown_semantic_35,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::unknown_semantic);
  assert_operation({0xF8U, 0x00U, 0x27U, 0x28U}, wfrac_protocol::OperationSemanticKind::home_leave_extension,
                    wfrac_protocol::OperationScope::unknown, wfrac_protocol::OperationValueQuality::special_extension);
  assert_operation({0xFEU, 0x29U, 0x2AU, 0x2BU}, wfrac_protocol::OperationSemanticKind::unknown,
                    wfrac_protocol::OperationScope::unknown,
                    wfrac_protocol::OperationValueQuality::unknown_semantic, false);
  assert_operation({0x05U, 0x2CU, 0x2DU, 0x2EU}, wfrac_protocol::OperationSemanticKind::unknown,
                    wfrac_protocol::OperationScope::unknown,
                    wfrac_protocol::OperationValueQuality::unknown_semantic, false);
  assert_operation({0x0CU, 0x2FU, 0x30U, 0x31U}, wfrac_protocol::OperationSemanticKind::unknown,
                    wfrac_protocol::OperationScope::unknown,
                    wfrac_protocol::OperationValueQuality::unknown_semantic, false);

  wfrac_protocol::AirconBlockView zero_segments{};
  wfrac_protocol::OperationSegment extracted{};
  assert(!wfrac_protocol::operation_segment_at(zero_segments, 0U, &extracted));
  const std::array<uint8_t, 4> one_segment{{0x94U, 0x32U, 0x33U, 0x34U}};
  wfrac_protocol::AirconBlockView one_block{};
  one_block.segment_count = 1U;
  one_block.segments = {one_segment.data(), one_segment.size()};
  assert(wfrac_protocol::operation_segment_at(one_block, 0U, &extracted));
  assert(extracted.code == 0x94U && extracted.op1 == 0x32U && extracted.op2 == 0x33U && extracted.op3 == 0x34U);
  assert(!wfrac_protocol::operation_segment_at(one_block, 1U, &extracted));
  const std::array<uint8_t, 8> multiple_segments{{0x11U, 0x35U, 0x36U, 0x37U, 0x82U, 0x38U, 0x39U, 0x3AU}};
  wfrac_protocol::AirconBlockView multiple_blocks{};
  multiple_blocks.segment_count = 2U;
  multiple_blocks.segments = {multiple_segments.data(), multiple_segments.size()};
  assert(wfrac_protocol::operation_segment_at(multiple_blocks, 0U, &extracted));
  assert(extracted.code == 0x11U && extracted.op3 == 0x37U);
  assert(wfrac_protocol::operation_segment_at(multiple_blocks, 1U, &extracted));
  assert(extracted.code == 0x82U && extracted.op1 == 0x38U && extracted.op2 == 0x39U && extracted.op3 == 0x3AU);
  assert(!wfrac_protocol::operation_segment_at(multiple_blocks, 2U, &extracted));
  multiple_blocks.segments.size = 7U;
  assert(!wfrac_protocol::operation_segment_at(multiple_blocks, 1U, &extracted));

  wfrac_protocol::CommandState safe_state = wfrac_protocol::make_safe_command_state();
  std::array<uint8_t, 1041> serialized{};
  std::size_t encoded_size = 0U;
  assert(wfrac_protocol::encoded_aircon_block_size(0U) == 21U);
  assert(wfrac_protocol::encoded_aircon_block_size(255U) == 1041U);
  assert(wfrac_protocol::encoded_aircon_block_size(256U) == 0U);
  assert(wfrac_protocol::serialize_aircon_block(safe_state, nullptr, 0U,
                                                {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::none);
  assert(encoded_size == 21U && serialized[18] == 0U && serialized[5] == 0xFFU);
  uint16_t serialized_crc = static_cast<uint16_t>(serialized[19]) |
                            (static_cast<uint16_t>(serialized[20]) << 8U);
  assert(serialized_crc == wfrac_protocol::crc16_ccitt(serialized.data(), 19U));

  const wfrac_protocol::OperationSegment one_raw{0xFEU, 0x10U, 0x20U, 0xA5U};
  assert(wfrac_protocol::serialize_aircon_block(safe_state, &one_raw, 1U,
                                                {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::none);
  assert(encoded_size == 25U && serialized[18] == 1U);
  assert(serialized[19] == 0xFEU && serialized[20] == 0x10U && serialized[21] == 0x20U && serialized[22] == 0xA5U);
  serialized_crc = static_cast<uint16_t>(serialized[23]) | (static_cast<uint16_t>(serialized[24]) << 8U);
  assert(serialized_crc == wfrac_protocol::crc16_ccitt(serialized.data(), 23U));

  const std::array<wfrac_protocol::OperationSegment, 3> three_raw{{
      {0x11U, 0xFFU, 0xFFU, 0x01U}, {0x90U, 0xFFU, 0xFFU, 0x02U}, {0x85U, 0xFFU, 0xFFU, 0x03U}}};
  assert(wfrac_protocol::serialize_aircon_block(safe_state, three_raw.data(), three_raw.size(),
                                                {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::none);
  assert(encoded_size == 33U && serialized[18] == 3U);
  assert(serialized[19] == 0x11U && serialized[23] == 0x90U && serialized[27] == 0x85U);
  assert(serialized[22] == 0x01U && serialized[26] == 0x02U && serialized[30] == 0x03U);
  assert(wfrac_protocol::serialize_aircon_block(safe_state, &one_raw, 1U, {serialized.data(), 24U}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::output_too_small);
  assert(wfrac_protocol::serialize_aircon_block(safe_state, nullptr, 1U,
                                                {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::invalid_segment_view);
  assert(wfrac_protocol::serialize_aircon_block(safe_state, &one_raw, 1U, {nullptr, serialized.size()}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::invalid_argument);
  assert(wfrac_protocol::serialize_aircon_block(safe_state, &one_raw, 1U,
                                                {serialized.data(), serialized.size()}, nullptr) ==
         wfrac_protocol::BlockEncodeError::invalid_argument);
  std::array<wfrac_protocol::OperationSegment, 255> max_raw{};
  max_raw[254] = {0xFEU, 0x01U, 0x02U, 0xA6U};
  assert(wfrac_protocol::serialize_aircon_block(safe_state, max_raw.data(), max_raw.size(),
                                                {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::BlockEncodeError::none);
  assert(encoded_size == 1041U);
  assert(serialized[18] == 255U);
  assert(serialized[1035] == 0xFEU && serialized[1036] == 0x01U && serialized[1037] == 0x02U &&
         serialized[1038] == 0xA6U);
  serialized_crc = static_cast<uint16_t>(serialized[1039]) | (static_cast<uint16_t>(serialized[1040]) << 8U);
  assert(serialized_crc == wfrac_protocol::crc16_ccitt(serialized.data(), 1039U));

  const auto query = wfrac_protocol::make_operation_query_segment(0xF8U);
  assert(query.code == 0xF8U && query.op1 == 0xFFU && query.op2 == 0xFFU && query.op3 == 0xFFU);
  assert(!wfrac_protocol::is_known_safe_operation_query_code(0x05U));
  const std::array<uint8_t, 16> safe_query_codes{{0x11U, 0x90U, 0x85U, 0x13U, 0x81U, 0x87U, 0x82U, 0xB1U,
                                                   0x7CU, 0x1EU, 0x1FU, 0x0DU, 0x21U, 0x32U, 0x34U, 0x35U}};
  for (const uint8_t code : safe_query_codes) assert(wfrac_protocol::is_known_safe_operation_query_code(code));
  for (const uint8_t code : {0x05U, 0x0CU, 0xF8U, 0xFEU, 0x80U, 0x94U}) {
    assert(!wfrac_protocol::is_known_safe_operation_query_code(code));
    assert(wfrac_protocol::build_safe_operation_query(code, {serialized.data(), serialized.size()}, &encoded_size) ==
           wfrac_protocol::OperationQueryError::invalid_code);
  }
  assert(wfrac_protocol::build_safe_operation_query(0x11U, {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::OperationQueryError::none);
  assert(encoded_size == 25U && serialized[5] == 0xFFU && serialized[18] == 1U);
  assert(serialized[19] == 0x11U && serialized[20] == 0xFFU && serialized[21] == 0xFFU && serialized[22] == 0xFFU);
  for (const std::size_t index : {13U, 14U, 16U, 17U}) assert(serialized[index] == 0U);
  serialized_crc = static_cast<uint16_t>(serialized[23]) | (static_cast<uint16_t>(serialized[24]) << 8U);
  assert(serialized_crc == wfrac_protocol::crc16_ccitt(serialized.data(), 23U));

  const std::array<uint8_t, 3> query_batch{{0x11U, 0x90U, 0x85U}};
  for (std::size_t count = 1U; count <= 3U; ++count) {
    assert(wfrac_protocol::build_safe_operation_query_batch(query_batch.data(), count,
                                                            {serialized.data(), serialized.size()}, &encoded_size) ==
           wfrac_protocol::OperationQueryError::none);
    assert(encoded_size == wfrac_protocol::encoded_aircon_block_size(count));
    assert(serialized[18] == count);
    for (std::size_t index = 0; index < count; ++index) {
      const std::size_t offset = 19U + index * 4U;
      assert(serialized[offset] == query_batch[index]);
      assert(serialized[offset + 1U] == 0xFFU && serialized[offset + 2U] == 0xFFU && serialized[offset + 3U] == 0xFFU);
    }
  }
  assert(wfrac_protocol::build_safe_operation_query_batch(nullptr, 0U,
                                                           {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::OperationQueryError::batch_size_out_of_range);
  const std::array<uint8_t, 4> too_many{{0x11U, 0x90U, 0x85U, 0x13U}};
  assert(wfrac_protocol::build_safe_operation_query_batch(too_many.data(), too_many.size(),
                                                           {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::OperationQueryError::batch_size_out_of_range);
  assert(wfrac_protocol::build_safe_operation_query_batch(nullptr, 1U,
                                                           {serialized.data(), serialized.size()}, &encoded_size) ==
         wfrac_protocol::OperationQueryError::invalid_argument);
  assert(wfrac_protocol::build_safe_operation_query(0x11U, {serialized.data(), 24U}, &encoded_size) ==
         wfrac_protocol::OperationQueryError::output_too_small);

  const auto assert_base64_vector = [](const char* plain, const char* expected) {
    const std::size_t plain_size = std::strlen(plain);
    const std::size_t expected_size = std::strlen(expected);
    assert(wfrac_protocol::base64_encoded_size(plain_size) == expected_size);
    std::array<uint8_t, 64> encoded{};
    std::size_t actual_size = 0U;
    assert(wfrac_protocol::base64_encode(
               {reinterpret_cast<const uint8_t*>(plain), plain_size}, {encoded.data(), encoded.size()}, &actual_size) ==
           wfrac_protocol::Base64Error::none);
    assert(actual_size == expected_size);
    assert(std::memcmp(encoded.data(), expected, expected_size) == 0);
    std::array<uint8_t, 64> decoded{};
    std::size_t decoded_size = 0U;
    assert(wfrac_protocol::base64_decode({encoded.data(), actual_size}, {decoded.data(), decoded.size()}, &decoded_size) ==
           wfrac_protocol::Base64Error::none);
    assert(decoded_size == plain_size);
    assert(std::memcmp(decoded.data(), plain, plain_size) == 0);
  };
  assert_base64_vector("", "");
  assert_base64_vector("f", "Zg==");
  assert_base64_vector("fo", "Zm8=");
  assert_base64_vector("foo", "Zm9v");
  assert_base64_vector("foob", "Zm9vYg==");
  assert_base64_vector("fooba", "Zm9vYmE=");
  assert_base64_vector("foobar", "Zm9vYmFy");
  const std::array<uint8_t, 4> binary_input{{0x00U, 0xFFU, 0x80U, 0x01U}};
  std::array<uint8_t, 16> binary_encoded{};
  std::size_t binary_encoded_size = 0U;
  assert(wfrac_protocol::base64_encode({binary_input.data(), binary_input.size()},
                                       {binary_encoded.data(), binary_encoded.size()}, &binary_encoded_size) ==
         wfrac_protocol::Base64Error::none);
  std::array<uint8_t, 4> binary_decoded{};
  std::size_t binary_decoded_size = 0U;
  assert(wfrac_protocol::base64_decode({binary_encoded.data(), binary_encoded_size},
                                       {binary_decoded.data(), binary_decoded.size()}, &binary_decoded_size) ==
         wfrac_protocol::Base64Error::none);
  assert(binary_decoded_size == binary_input.size());
  assert(binary_decoded == binary_input);
  const auto invalid_b64 = [](const char* value, std::size_t size, wfrac_protocol::Base64Error expected_error) {
    std::array<uint8_t, 16> output{};
    std::size_t decoded_size = 0U;
    assert(wfrac_protocol::base64_decode({reinterpret_cast<const uint8_t*>(value), size},
                                         {output.data(), output.size()}, &decoded_size) == expected_error);
  };
  invalid_b64("Zg=", 3U, wfrac_protocol::Base64Error::invalid_length);
  invalid_b64("Zm$=", 4U, wfrac_protocol::Base64Error::invalid_character);
  invalid_b64("Z=8=", 4U, wfrac_protocol::Base64Error::invalid_padding);
  invalid_b64("Zg=A", 4U, wfrac_protocol::Base64Error::invalid_padding);
  invalid_b64("Zg===", 5U, wfrac_protocol::Base64Error::invalid_length);
  std::array<uint8_t, 2> too_small{};
  std::size_t invalid_size = 0U;
  assert(wfrac_protocol::base64_encode({reinterpret_cast<const uint8_t*>("foo"), 3U},
                                       {too_small.data(), too_small.size()}, &invalid_size) ==
         wfrac_protocol::Base64Error::output_too_small);
  assert(wfrac_protocol::base64_decode({reinterpret_cast<const uint8_t*>("Zg=="), 4U},
                                       {too_small.data(), 0U}, &invalid_size) ==
         wfrac_protocol::Base64Error::output_too_small);
  assert(wfrac_protocol::base64_encode({nullptr, 1U}, {too_small.data(), too_small.size()}, &invalid_size) ==
         wfrac_protocol::Base64Error::invalid_argument);
  assert(wfrac_protocol::base64_decode({nullptr, 4U}, {too_small.data(), too_small.size()}, &invalid_size) ==
         wfrac_protocol::Base64Error::invalid_argument);
  assert(wfrac_protocol::base64_decoded_size({reinterpret_cast<const uint8_t*>("Zg=="), 4U}, nullptr) ==
         wfrac_protocol::Base64Error::invalid_argument);

  const wfrac_protocol::CommandState command_state = wfrac_protocol::make_safe_command_state();
  wfrac_protocol::CommandState composed_receive_state{};
  for (std::size_t index = 0U; index < composed_receive_state.size(); ++index) {
    composed_receive_state[index] = static_cast<uint8_t>(index + 1U);
  }
  const wfrac_protocol::OperationSegment command_segment{0x11U, 0xFFU, 0xFFU, 0xFFU};
  const std::array<wfrac_protocol::OperationSegment, 2> receive_segments{{
      {0x80U, 0x10U, 0xAAU, 0xBBU}, {0x34U, 0x01U, 0x02U, 0xA5U}}};
  const wfrac_protocol::AirconBlockInput command_input{&command_state, &command_segment, 1U};
  const wfrac_protocol::AirconBlockInput receive_input{&composed_receive_state, receive_segments.data(), receive_segments.size()};
  std::array<uint8_t, 4096> aircon_raw{};
  std::size_t aircon_raw_size = 0U;
  assert(wfrac_protocol::encoded_aircon_stat_size(1U, 2U) == 54U);
  assert(wfrac_protocol::serialize_aircon_stat(command_input, receive_input,
                                               {aircon_raw.data(), aircon_raw.size()}, &aircon_raw_size) ==
         wfrac_protocol::AirconStatEncodeError::none);
  assert(aircon_raw_size == 54U);
  wfrac_protocol::AirconFrameView round_trip_frame{};
  assert(wfrac_protocol::parse_aircon_stat({aircon_raw.data(), aircon_raw_size}, &round_trip_frame) ==
         wfrac_protocol::FrameError::none);
  assert(round_trip_frame.command.state == command_state);
  assert(round_trip_frame.receive.state == composed_receive_state);
  assert(round_trip_frame.command.segment_count == 1U && round_trip_frame.receive.segment_count == 2U);
  assert(round_trip_frame.command.crc_valid && round_trip_frame.receive.crc_valid);
  assert(round_trip_frame.receive_offset == round_trip_frame.command.encoded_size);
  wfrac_protocol::OperationSegment round_trip_segment{};
  assert(wfrac_protocol::operation_segment_at(round_trip_frame.command, 0U, &round_trip_segment));
  assert(round_trip_segment.code == command_segment.code && round_trip_segment.op3 == command_segment.op3);
  for (std::size_t index = 0U; index < receive_segments.size(); ++index) {
    assert(wfrac_protocol::operation_segment_at(round_trip_frame.receive, index, &round_trip_segment));
    assert(round_trip_segment.code == receive_segments[index].code && round_trip_segment.op1 == receive_segments[index].op1 &&
           round_trip_segment.op2 == receive_segments[index].op2 && round_trip_segment.op3 == receive_segments[index].op3);
  }
  std::array<uint8_t, 4096> aircon_base64{};
  std::size_t aircon_base64_size = 0U;
  assert(wfrac_protocol::base64_encode({aircon_raw.data(), aircon_raw_size},
                                       {aircon_base64.data(), aircon_base64.size()}, &aircon_base64_size) ==
         wfrac_protocol::Base64Error::none);
  std::array<uint8_t, 4096> decoded_aircon_raw{};
  std::size_t decoded_aircon_raw_size = 0U;
  assert(wfrac_protocol::base64_decode({aircon_base64.data(), aircon_base64_size},
                                       {decoded_aircon_raw.data(), decoded_aircon_raw.size()}, &decoded_aircon_raw_size) ==
         wfrac_protocol::Base64Error::none);
  assert(decoded_aircon_raw_size == aircon_raw_size);
  for (std::size_t index = 0U; index < aircon_raw_size; ++index) assert(decoded_aircon_raw[index] == aircon_raw[index]);
  assert(wfrac_protocol::parse_aircon_stat({decoded_aircon_raw.data(), decoded_aircon_raw_size}, &round_trip_frame) ==
         wfrac_protocol::FrameError::none);
  assert(round_trip_frame.command.state == command_state && round_trip_frame.receive.state == composed_receive_state);
  assert(round_trip_frame.command.crc_valid && round_trip_frame.receive.crc_valid);

  const auto assert_composition = [&](const wfrac_protocol::OperationSegment* command_segments, std::size_t command_count,
                                      const wfrac_protocol::OperationSegment* receive_segment_data,
                                      std::size_t receive_count) {
    const wfrac_protocol::AirconBlockInput command_case{&command_state, command_segments, command_count};
    const wfrac_protocol::AirconBlockInput receive_case{&composed_receive_state, receive_segment_data, receive_count};
    std::size_t size = 0U;
    assert(wfrac_protocol::serialize_aircon_stat(command_case, receive_case,
                                                 {aircon_raw.data(), aircon_raw.size()}, &size) ==
           wfrac_protocol::AirconStatEncodeError::none);
    assert(size == wfrac_protocol::encoded_aircon_stat_size(command_count, receive_count));
  };
  assert_composition(nullptr, 0U, nullptr, 0U);
  assert_composition(nullptr, 0U, receive_segments.data(), receive_segments.size());
  assert_composition(&command_segment, 1U, nullptr, 0U);
  assert(wfrac_protocol::serialize_aircon_stat(command_input, receive_input, {aircon_raw.data(), 53U}, &aircon_raw_size) ==
         wfrac_protocol::AirconStatEncodeError::output_too_small);
  assert(wfrac_protocol::serialize_aircon_stat(command_input,
                                               {&composed_receive_state, nullptr, 0U},
                                               {aircon_raw.data(), aircon_raw.size()}, &aircon_raw_size) ==
         wfrac_protocol::AirconStatEncodeError::none);

  std::array<wfrac_protocol::OperationSegment, 255> maximum_segments{};
  maximum_segments[254] = {0xFEU, 0x01U, 0x02U, 0xA6U};
  const wfrac_protocol::AirconBlockInput maximum_command{&command_state, maximum_segments.data(), maximum_segments.size()};
  const wfrac_protocol::AirconBlockInput maximum_receive{&composed_receive_state, maximum_segments.data(), maximum_segments.size()};
  const std::size_t maximum_raw_size = wfrac_protocol::encoded_aircon_stat_size(255U, 255U);
  assert(maximum_raw_size == 2082U);
  assert(wfrac_protocol::serialize_aircon_stat(maximum_command, maximum_receive,
                                               {aircon_raw.data(), aircon_raw.size()}, &aircon_raw_size) ==
         wfrac_protocol::AirconStatEncodeError::none);
  assert(aircon_raw_size == maximum_raw_size);
  assert(wfrac_protocol::base64_encoded_size(maximum_raw_size) == 2776U);
  return 0;
}
