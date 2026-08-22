#include "wfrac_protocol/aircon_block.hpp"

#include "wfrac_protocol/crc16.hpp"

namespace wfrac_protocol {

std::size_t encoded_aircon_block_size(std::size_t segment_count) {
  if (segment_count > 0xFFU) return 0U;
  return kAirconStateBytes + 1U + segment_count * kAirconSegmentBytes + kAirconCrcBytes;
}

BlockEncodeError serialize_aircon_block(const CommandState& state, const OperationSegment* segments,
                                        std::size_t segment_count, MutableByteView output, std::size_t* encoded_size) {
  if (encoded_size == nullptr) return BlockEncodeError::invalid_argument;
  *encoded_size = 0U;
  if (segment_count > 0xFFU) return BlockEncodeError::segment_count_overflow;
  if (segment_count > 0U && segments == nullptr) return BlockEncodeError::invalid_segment_view;
  if (output.data == nullptr) return BlockEncodeError::invalid_argument;
  const std::size_t required = encoded_aircon_block_size(segment_count);
  if (output.size < required) return BlockEncodeError::output_too_small;

  for (std::size_t index = 0; index < kAirconStateBytes; ++index) output.data[index] = state[index];
  output.data[kAirconStateBytes] = static_cast<uint8_t>(segment_count);
  const std::size_t segment_offset = kAirconStateBytes + 1U;
  for (std::size_t segment_index = 0; segment_index < segment_count; ++segment_index) {
    const std::size_t offset = segment_offset + segment_index * kAirconSegmentBytes;
    output.data[offset] = segments[segment_index].code;
    output.data[offset + 1U] = segments[segment_index].op1;
    output.data[offset + 2U] = segments[segment_index].op2;
    output.data[offset + 3U] = segments[segment_index].op3;
  }
  const uint16_t crc = crc16_ccitt(output.data, required - kAirconCrcBytes);
  output.data[required - 2U] = static_cast<uint8_t>(crc & 0xFFU);
  output.data[required - 1U] = static_cast<uint8_t>(crc >> 8U);
  *encoded_size = required;
  return BlockEncodeError::none;
}

}  // namespace wfrac_protocol
