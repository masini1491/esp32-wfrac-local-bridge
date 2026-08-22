#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace wfrac_protocol {

constexpr std::size_t kAirconStateBytes = 18;
constexpr std::size_t kAirconSegmentBytes = 4;
constexpr std::size_t kAirconCrcBytes = 2;

enum class FrameError {
  none,
  too_short,
  truncated_segments,
  missing_crc,
  invalid_layout,
};

struct ByteView {
  const uint8_t* data = nullptr;
  std::size_t size = 0;
};

struct AirconBlockView {
  std::array<uint8_t, kAirconStateBytes> state{};
  uint8_t segment_count = 0;
  ByteView segments{};
  uint16_t stored_crc = 0;
  uint16_t calculated_crc = 0;
  bool crc_valid = false;
  std::size_t encoded_size = 0;
};

struct AirconFrameView {
  AirconBlockView command{};
  AirconBlockView receive{};
  std::size_t receive_offset = 0;
};

FrameError parse_aircon_stat(ByteView raw, AirconFrameView* output);

}  // namespace wfrac_protocol
