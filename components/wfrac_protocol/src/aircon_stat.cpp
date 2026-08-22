#include "wfrac_protocol/aircon_stat.hpp"

#include <limits>

namespace wfrac_protocol {

std::size_t encoded_aircon_stat_size(std::size_t command_segment_count, std::size_t receive_segment_count) {
  const std::size_t command_size = encoded_aircon_block_size(command_segment_count);
  const std::size_t receive_size = encoded_aircon_block_size(receive_segment_count);
  if (command_size == 0U || receive_size == 0U || command_size > std::numeric_limits<std::size_t>::max() - receive_size) {
    return 0U;
  }
  return command_size + receive_size;
}

AirconStatEncodeError serialize_aircon_stat(const AirconBlockInput& command, const AirconBlockInput& receive,
                                            MutableByteView output, std::size_t* encoded_size) {
  if (encoded_size == nullptr || command.state == nullptr || receive.state == nullptr) {
    return AirconStatEncodeError::invalid_argument;
  }
  *encoded_size = 0U;
  if ((command.segment_count > 0U && command.segments == nullptr) ||
      (receive.segment_count > 0U && receive.segments == nullptr)) {
    return AirconStatEncodeError::invalid_argument;
  }
  const std::size_t command_size = encoded_aircon_block_size(command.segment_count);
  const std::size_t receive_size = encoded_aircon_block_size(receive.segment_count);
  const std::size_t total_size = encoded_aircon_stat_size(command.segment_count, receive.segment_count);
  if (command_size == 0U || receive_size == 0U || total_size == 0U) return AirconStatEncodeError::size_overflow;
  if (output.data == nullptr) return AirconStatEncodeError::invalid_argument;
  if (output.size < total_size) return AirconStatEncodeError::output_too_small;

  const BlockEncodeError command_error = serialize_aircon_block(
      *command.state, command.segments, command.segment_count, {output.data, command_size}, encoded_size);
  if (command_error != BlockEncodeError::none) return AirconStatEncodeError::command_block_failure;
  std::size_t receive_encoded_size = 0U;
  const BlockEncodeError receive_error = serialize_aircon_block(
      *receive.state, receive.segments, receive.segment_count, {output.data + command_size, receive_size},
      &receive_encoded_size);
  if (receive_error != BlockEncodeError::none) return AirconStatEncodeError::receive_block_failure;
  *encoded_size = command_size + receive_encoded_size;
  return AirconStatEncodeError::none;
}

}  // namespace wfrac_protocol
