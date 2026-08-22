#include "wfrac_protocol/aircon_frame.hpp"

#include "wfrac_protocol/crc16.hpp"

namespace wfrac_protocol {
namespace {

FrameError parse_block(ByteView raw, AirconBlockView* output) {
  const std::size_t prefix_size = kAirconStateBytes + 1;
  if (raw.size < prefix_size) {
    return FrameError::too_short;
  }

  AirconBlockView block{};
  for (std::size_t index = 0; index < kAirconStateBytes; ++index) {
    block.state[index] = raw.data[index];
  }
  block.segment_count = raw.data[kAirconStateBytes];
  const std::size_t segment_bytes = static_cast<std::size_t>(block.segment_count) * kAirconSegmentBytes;
  if (segment_bytes > raw.size - prefix_size) {
    return FrameError::truncated_segments;
  }
  const std::size_t crc_offset = prefix_size + segment_bytes;
  if (raw.size < crc_offset + kAirconCrcBytes) {
    return FrameError::missing_crc;
  }

  block.segments = {raw.data + prefix_size, segment_bytes};
  block.stored_crc = static_cast<uint16_t>(raw.data[crc_offset]) |
                     (static_cast<uint16_t>(raw.data[crc_offset + 1]) << 8);
  block.calculated_crc = crc16_ccitt(raw.data, crc_offset);
  block.crc_valid = block.stored_crc == block.calculated_crc;
  block.encoded_size = crc_offset + kAirconCrcBytes;
  *output = block;
  return FrameError::none;
}

}  // namespace

FrameError parse_aircon_stat(ByteView raw, AirconFrameView* output) {
  if (output == nullptr || raw.data == nullptr) {
    return FrameError::invalid_layout;
  }

  AirconFrameView frame{};
  FrameError error = parse_block(raw, &frame.command);
  if (error != FrameError::none) {
    return error;
  }
  frame.receive_offset = frame.command.encoded_size;
  if (frame.receive_offset >= raw.size) {
    return FrameError::missing_crc;
  }
  error = parse_block({raw.data + frame.receive_offset, raw.size - frame.receive_offset}, &frame.receive);
  if (error != FrameError::none) {
    return error;
  }
  if (frame.receive_offset + frame.receive.encoded_size != raw.size) {
    return FrameError::invalid_layout;
  }
  *output = frame;
  return FrameError::none;
}

}  // namespace wfrac_protocol
