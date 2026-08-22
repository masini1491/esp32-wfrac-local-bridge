#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_protocol/aircon_frame.hpp"
#include "wfrac_protocol/aircon_state.hpp"
#include "wfrac_protocol/operation_data.hpp"

namespace wfrac_protocol {

enum class BlockEncodeError {
  none,
  invalid_argument,
  output_too_small,
  segment_count_overflow,
  invalid_segment_view,
};

std::size_t encoded_aircon_block_size(std::size_t segment_count);
BlockEncodeError serialize_aircon_block(const CommandState& state, const OperationSegment* segments,
                                        std::size_t segment_count, ByteView output, std::size_t* encoded_size);

}  // namespace wfrac_protocol
