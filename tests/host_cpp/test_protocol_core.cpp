#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#include "wfrac_protocol/aircon_frame.hpp"
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
  return 0;
}
