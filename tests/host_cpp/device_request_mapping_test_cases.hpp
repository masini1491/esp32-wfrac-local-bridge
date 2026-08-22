#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <utility>

#include "wfrac_device_model/protocol_request_mapping.hpp"
#include "wfrac_protocol/operation_data.hpp"

namespace device_request_mapping_tests {

using wfrac_device_model::CommandIntent;
using wfrac_device_model::FanSetting;
using wfrac_device_model::HvacMode;
using wfrac_device_model::PowerState;
using wfrac_device_model::ProtocolRequestMappingError;
using wfrac_device_model::SetFanIntent;
using wfrac_device_model::SetModeIntent;
using wfrac_device_model::SetPowerIntent;
using wfrac_device_model::SetTargetTemperatureIntent;

wfrac_protocol::AirconBlockView make_receive_baseline(const uint8_t* segments, std::size_t segment_bytes) {
  wfrac_protocol::AirconBlockView baseline{};
  baseline.state = {0xC3U, 0x44U, 0xD0U, 0xA2U, 0x2CU, 0x55U, 0x66U, 0x77U, 0xA8U,
                    0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0x0FU, 0x10U, 0x11U};
  baseline.segment_count = static_cast<uint8_t>(segment_bytes / wfrac_protocol::kAirconSegmentBytes);
  baseline.segments = {segments, segment_bytes};
  baseline.stored_crc = 0x1234U;
  baseline.calculated_crc = 0x1234U;
  baseline.crc_valid = true;
  baseline.encoded_size = wfrac_protocol::kAirconStateBytes + 1U + segment_bytes + wfrac_protocol::kAirconCrcBytes;
  return baseline;
}

void assert_sentinel(const wfrac_protocol::AirconBlockView& block) {
  assert(block.segment_count == 1U);
  wfrac_protocol::OperationSegment segment{};
  assert(wfrac_protocol::operation_segment_at(block, 0U, &segment));
  assert(segment.code == 0xFFU && segment.op1 == 0xFFU && segment.op2 == 0xFFU && segment.op3 == 0xFFU);
}

void assert_request_parses(const CommandIntent& intent, const wfrac_protocol::AirconBlockView& baseline,
                           std::array<uint8_t, 64>* raw, wfrac_protocol::AirconFrameView* frame) {
  std::size_t encoded_size = 0U;
  assert(wfrac_device_model::build_control_aircon_stat(intent, baseline, {raw->data(), raw->size()}, &encoded_size) ==
         ProtocolRequestMappingError::none);
  assert(encoded_size == 50U);
  assert(wfrac_protocol::parse_aircon_stat({raw->data(), encoded_size}, frame) == wfrac_protocol::FrameError::none);
  assert(frame->command.crc_valid && frame->receive.crc_valid);
  assert(frame->receive_offset == 25U);
  assert_sentinel(frame->command);
  assert_sentinel(frame->receive);
}

void run_device_request_mapping_tests() {
  const std::array<uint8_t, 8> response_segments{{0x80U, 0x10U, 0x55U, 0xAAU, 0x94U, 0x10U, 0x01U, 0x02U}};
  const wfrac_protocol::AirconBlockView baseline = make_receive_baseline(response_segments.data(), response_segments.size());

  {
    wfrac_protocol::AirconBlockView invalid_baseline = baseline;
    invalid_baseline.crc_valid = false;
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_control_aircon_stat(SetPowerIntent{PowerState::on}, invalid_baseline,
                                                         {raw.data(), raw.size()}, &encoded_size) ==
           ProtocolRequestMappingError::invalid_receive_baseline);
    assert(encoded_size == 0U);
  }

  {
    std::array<uint8_t, 64> raw{};
    wfrac_protocol::AirconFrameView frame{};
    assert_request_parses(SetPowerIntent{PowerState::on}, baseline, &raw, &frame);
    wfrac_protocol::CommandState expected_command{};
    assert(wfrac_device_model::map_command_intent_to_protocol(SetPowerIntent{PowerState::on}, &expected_command) ==
           wfrac_device_model::ProtocolCommandMappingError::none);
    expected_command[8] = 0x08U;
    assert(frame.command.state == expected_command);
    assert(frame.command.state[2] == 0x03U && frame.command.state[5] == 0xFFU && frame.command.state[8] == 0x08U);
    auto expected_receive = baseline.state;
    expected_receive[2] = static_cast<uint8_t>((expected_receive[2] & 0xFCU) | 0x01U);
    assert(frame.receive.state == expected_receive);
  }

  for (const auto& item : std::array<std::pair<HvacMode, uint8_t>, 5>{{
           {HvacMode::auto_mode, 0x00U}, {HvacMode::cool, 0x08U}, {HvacMode::heat, 0x10U},
           {HvacMode::fan, 0x0CU}, {HvacMode::dry, 0x04U}}}) {
    std::array<uint8_t, 64> raw{};
    wfrac_protocol::AirconFrameView frame{};
    assert_request_parses(SetModeIntent{item.first}, baseline, &raw, &frame);
    wfrac_protocol::CommandState expected_command{};
    assert(wfrac_device_model::map_command_intent_to_protocol(SetModeIntent{item.first}, &expected_command) ==
           wfrac_device_model::ProtocolCommandMappingError::none);
    expected_command[8] = 0x08U;
    assert(frame.command.state == expected_command);
    auto expected_receive = baseline.state;
    expected_receive[2] = static_cast<uint8_t>((expected_receive[2] & 0xC3U) | item.second);
    assert(frame.receive.state == expected_receive);
  }

  for (const auto& item : std::array<std::pair<FanSetting, uint8_t>, 5>{{
           {FanSetting::auto_mode, 0x07U}, {FanSetting::speed1, 0x00U}, {FanSetting::speed2, 0x01U},
           {FanSetting::speed3, 0x02U}, {FanSetting::speed4, 0x06U}}}) {
    std::array<uint8_t, 64> raw{};
    wfrac_protocol::AirconFrameView frame{};
    assert_request_parses(SetFanIntent{item.first}, baseline, &raw, &frame);
    wfrac_protocol::CommandState expected_command{};
    assert(wfrac_device_model::map_command_intent_to_protocol(SetFanIntent{item.first}, &expected_command) ==
           wfrac_device_model::ProtocolCommandMappingError::none);
    expected_command[8] = 0x08U;
    assert(frame.command.state == expected_command);
    auto expected_receive = baseline.state;
    expected_receive[3] = static_cast<uint8_t>((expected_receive[3] & 0xF0U) | item.second);
    assert(frame.receive.state == expected_receive);
  }

  {
    std::array<uint8_t, 64> raw{};
    wfrac_protocol::AirconFrameView frame{};
    assert_request_parses(SetTargetTemperatureIntent{2050}, baseline, &raw, &frame);
    wfrac_protocol::CommandState expected_command{};
    assert(wfrac_device_model::map_command_intent_to_protocol(SetTargetTemperatureIntent{2050}, &expected_command) ==
           wfrac_device_model::ProtocolCommandMappingError::none);
    expected_command[8] = 0x08U;
    assert(frame.command.state == expected_command);
    assert(frame.command.state[4] == 0xA9U && frame.command.state[5] == 0xFFU);
    auto expected_receive = baseline.state;
    expected_receive[4] = 0x29U;
    assert(frame.receive.state == expected_receive);
  }

  {
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_control_aircon_stat(SetTargetTemperatureIntent{2025}, baseline,
                                                         {raw.data(), raw.size()}, &encoded_size) ==
           ProtocolRequestMappingError::command_mapping_error);
    assert(encoded_size == 0U);
  }

  {
    std::array<uint8_t, 49> too_small{};
    too_small.fill(0xAAU);
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_control_aircon_stat(SetPowerIntent{PowerState::on}, baseline,
                                                         {too_small.data(), too_small.size()}, &encoded_size) ==
           ProtocolRequestMappingError::request_encode_error);
    assert(encoded_size == 0U);
    for (const uint8_t byte : too_small) assert(byte == 0xAAU);
  }

  {
    const uint8_t byte8_values[] = {0xA0U, 0xA8U};
    for (const uint8_t byte8 : byte8_values) {
      wfrac_protocol::AirconBlockView byte8_baseline = baseline;
      byte8_baseline.state[8] = byte8;
      std::array<uint8_t, 64> raw{};
      wfrac_protocol::AirconFrameView frame{};
      assert_request_parses(SetPowerIntent{PowerState::on}, byte8_baseline, &raw, &frame);
      assert(frame.command.state[8] == static_cast<uint8_t>(byte8 & 0x08U));
    }
  }

  {
    const auto state_before = baseline.state;
    const auto segment_count_before = baseline.segment_count;
    const auto segments_before = baseline.segments;
    const auto stored_crc_before = baseline.stored_crc;
    const auto calculated_crc_before = baseline.calculated_crc;
    const auto crc_valid_before = baseline.crc_valid;
    const auto encoded_size_before = baseline.encoded_size;
    std::array<uint8_t, 64> raw{};
    wfrac_protocol::AirconFrameView frame{};
    assert_request_parses(SetPowerIntent{PowerState::on}, baseline, &raw, &frame);
    assert(baseline.state == state_before && baseline.segment_count == segment_count_before);
    assert(baseline.segments.data == segments_before.data && baseline.segments.size == segments_before.size);
    assert(baseline.stored_crc == stored_crc_before && baseline.calculated_crc == calculated_crc_before &&
           baseline.crc_valid == crc_valid_before && baseline.encoded_size == encoded_size_before);
    assert(frame.receive.segments.size == wfrac_protocol::kAirconSegmentBytes);
    assert(frame.receive.segments.data[0] == 0xFFU);
  }
}

}  // namespace device_request_mapping_tests
