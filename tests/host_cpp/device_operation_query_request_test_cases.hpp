#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "wfrac_device_model/protocol_request_mapping.hpp"
#include "wfrac_protocol/operation_data.hpp"

namespace device_operation_query_request_tests {

using wfrac_device_model::OperationQueryRequestError;

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

void assert_segment(const wfrac_protocol::AirconBlockView& block, std::size_t index, uint8_t code) {
  wfrac_protocol::OperationSegment segment{};
  assert(wfrac_protocol::operation_segment_at(block, index, &segment));
  assert(segment.code == code && segment.op1 == 0xFFU && segment.op2 == 0xFFU && segment.op3 == 0xFFU);
}

void run_device_operation_query_request_tests() {
  const std::array<uint8_t, 8> response_segments{{0x80U, 0x10U, 0x55U, 0xAAU, 0x94U, 0x10U, 0x01U, 0x02U}};
  const wfrac_protocol::AirconBlockView baseline = make_receive_baseline(response_segments.data(), response_segments.size());

  {
    const uint8_t code = 0x11U;
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 0U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(&code, 1U, baseline, {raw.data(), raw.size()},
                                                                 &encoded_size) == OperationQueryRequestError::none);
    assert(encoded_size == 50U);
    wfrac_protocol::AirconFrameView frame{};
    assert(wfrac_protocol::parse_aircon_stat({raw.data(), encoded_size}, &frame) == wfrac_protocol::FrameError::none);
    assert(frame.command.crc_valid && frame.receive.crc_valid && frame.receive_offset == 25U);
    assert(frame.command.state[5] == 0xFFU && frame.command.state[8] == 0x08U);
    assert((frame.command.state[2] & 0x03U) == 0U && (frame.command.state[2] & 0x3CU) == 0U);
    assert((frame.command.state[3] & 0x0FU) == 0U && (frame.command.state[4] & 0x80U) == 0U);
    assert(frame.command.segment_count == 1U);
    assert_segment(frame.command, 0U, 0x11U);
    assert(frame.receive.state == baseline.state && frame.receive.segment_count == 1U);
    assert_segment(frame.receive, 0U, 0xFFU);
  }

  for (const auto& batch : {std::array<uint8_t, 2>{{0x11U, 0x90U}}, std::array<uint8_t, 2>{{0x85U, 0x11U}}}) {
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 0U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(batch.data(), batch.size(), baseline,
                                                                 {raw.data(), raw.size()}, &encoded_size) ==
           OperationQueryRequestError::none);
    assert(encoded_size == 54U);
    wfrac_protocol::AirconFrameView frame{};
    assert(wfrac_protocol::parse_aircon_stat({raw.data(), encoded_size}, &frame) == wfrac_protocol::FrameError::none);
    assert(frame.command.segment_count == batch.size());
    assert_segment(frame.command, 0U, batch[0]);
    assert_segment(frame.command, 1U, batch[1]);
  }

  {
    const std::array<uint8_t, 3> codes{{0x11U, 0x90U, 0x85U}};
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 0U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(codes.data(), codes.size(), baseline,
                                                                 {raw.data(), raw.size()}, &encoded_size) ==
           OperationQueryRequestError::none);
    assert(encoded_size == 58U);
    wfrac_protocol::AirconFrameView frame{};
    assert(wfrac_protocol::parse_aircon_stat({raw.data(), encoded_size}, &frame) == wfrac_protocol::FrameError::none);
    for (std::size_t index = 0; index < codes.size(); ++index) assert_segment(frame.command, index, codes[index]);
  }

  for (const uint8_t code : {0x11U, 0x90U, 0x85U, 0x13U, 0x81U, 0x87U, 0x82U, 0xB1U,
                             0x7CU, 0x1EU, 0x1FU, 0x0DU, 0x21U, 0x32U, 0x34U, 0x35U}) {
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 0U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(&code, 1U, baseline, {raw.data(), raw.size()},
                                                                 &encoded_size) == OperationQueryRequestError::none);
    assert(encoded_size == 50U);
  }

  for (const uint8_t code : {0x05U, 0x0CU, 0x80U, 0x94U, 0xF8U, 0xFEU, 0xFFU}) {
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(&code, 1U, baseline, {raw.data(), raw.size()},
                                                                 &encoded_size) ==
           OperationQueryRequestError::invalid_query_code);
    assert(encoded_size == 0U);
  }

  {
    const std::array<uint8_t, 4> four_codes{{0x11U, 0x90U, 0x85U, 0x13U}};
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(four_codes.data(), 0U, baseline,
                                                                 {raw.data(), raw.size()}, &encoded_size) ==
           OperationQueryRequestError::batch_size_out_of_range);
    assert(encoded_size == 0U);
    assert(wfrac_device_model::build_operation_query_aircon_stat(four_codes.data(), four_codes.size(), baseline,
                                                                 {raw.data(), raw.size()}, &encoded_size) ==
           OperationQueryRequestError::batch_size_out_of_range);
    assert(encoded_size == 0U);
  }

  {
    const std::array<uint8_t, 3> mixed{{0x11U, 0x90U, 0xF8U}};
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(mixed.data(), mixed.size(), baseline,
                                                                 {raw.data(), raw.size()}, &encoded_size) ==
           OperationQueryRequestError::invalid_query_code);
    assert(encoded_size == 0U);
  }

  {
    const std::array<uint8_t, 2> duplicate{{0x11U, 0x11U}};
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 0U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(duplicate.data(), duplicate.size(), baseline,
                                                                 {raw.data(), raw.size()}, &encoded_size) ==
           OperationQueryRequestError::none);
    wfrac_protocol::AirconFrameView frame{};
    assert(wfrac_protocol::parse_aircon_stat({raw.data(), encoded_size}, &frame) == wfrac_protocol::FrameError::none);
    assert_segment(frame.command, 0U, 0x11U);
    assert_segment(frame.command, 1U, 0x11U);
  }

  {
    wfrac_protocol::AirconBlockView invalid_baseline = baseline;
    invalid_baseline.crc_valid = false;
    const uint8_t code = 0x11U;
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(&code, 1U, invalid_baseline, {raw.data(), raw.size()},
                                                                 &encoded_size) ==
           OperationQueryRequestError::invalid_receive_baseline);
    assert(encoded_size == 0U);
  }

  {
    const uint8_t code = 0x11U;
    std::array<uint8_t, 49> too_small{};
    too_small.fill(0xAAU);
    std::size_t encoded_size = 99U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(&code, 1U, baseline,
                                                                 {too_small.data(), too_small.size()}, &encoded_size) ==
           OperationQueryRequestError::request_encode_error);
    assert(encoded_size == 0U);
    for (const uint8_t byte : too_small) assert(byte == 0xAAU);
  }

  {
    const std::array<uint8_t, 2> codes{{0x11U, 0x90U}};
    std::array<uint8_t, 64> block{};
    std::array<uint8_t, 64> raw{};
    std::size_t block_size = 0U;
    std::size_t raw_size = 0U;
    assert(wfrac_protocol::build_safe_operation_query_batch(codes.data(), codes.size(), {block.data(), block.size()},
                                                            &block_size) == wfrac_protocol::OperationQueryError::none);
    assert(wfrac_device_model::build_operation_query_aircon_stat(codes.data(), codes.size(), baseline,
                                                                 {raw.data(), raw.size()}, &raw_size) ==
           OperationQueryRequestError::none);
    wfrac_protocol::AirconFrameView frame{};
    assert(wfrac_protocol::parse_aircon_stat({raw.data(), raw_size}, &frame) == wfrac_protocol::FrameError::none);
    assert(block_size == frame.command.encoded_size);
    for (std::size_t index = 0; index < wfrac_protocol::kAirconStateBytes; ++index) {
      const uint8_t expected = index == 8U ? 0x08U : block[index];
      assert(frame.command.state[index] == expected);
    }
    for (std::size_t index = 0; index < frame.command.segments.size; ++index) {
      assert(frame.command.segments.data[index] == block[wfrac_protocol::kAirconStateBytes + 1U + index]);
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
    const uint8_t code = 0x11U;
    std::array<uint8_t, 64> raw{};
    std::size_t encoded_size = 0U;
    assert(wfrac_device_model::build_operation_query_aircon_stat(&code, 1U, baseline, {raw.data(), raw.size()},
                                                                 &encoded_size) == OperationQueryRequestError::none);
    assert(baseline.state == state_before && baseline.segment_count == segment_count_before);
    assert(baseline.segments.data == segments_before.data && baseline.segments.size == segments_before.size);
    assert(baseline.stored_crc == stored_crc_before && baseline.calculated_crc == calculated_crc_before &&
           baseline.crc_valid == crc_valid_before && baseline.encoded_size == encoded_size_before);
  }
}

}  // namespace device_operation_query_request_tests
