#pragma once

#include <cstddef>

#include "wfrac_protocol/aircon_block.hpp"

namespace wfrac_protocol {

struct AirconBlockInput {
  const CommandState* state = nullptr;
  const OperationSegment* segments = nullptr;
  std::size_t segment_count = 0;
};

enum class AirconStatEncodeError {
  none,
  invalid_argument,
  command_block_failure,
  receive_block_failure,
  output_too_small,
  size_overflow,
};

std::size_t encoded_aircon_stat_size(std::size_t command_segment_count, std::size_t receive_segment_count);
AirconStatEncodeError serialize_aircon_stat(const AirconBlockInput& command, const AirconBlockInput& receive,
                                            MutableByteView output, std::size_t* encoded_size);

}  // namespace wfrac_protocol
