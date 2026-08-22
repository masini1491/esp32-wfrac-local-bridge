#pragma once

#include <cstddef>

#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_protocol {

enum class Base64Error {
  none,
  invalid_argument,
  invalid_length,
  invalid_character,
  invalid_padding,
  output_too_small,
  size_overflow,
};

std::size_t base64_encoded_size(std::size_t input_size);
Base64Error base64_decoded_size(ByteView encoded, std::size_t* decoded_size);
Base64Error base64_encode(ByteView input, MutableByteView output, std::size_t* encoded_size);
Base64Error base64_decode(ByteView encoded, MutableByteView output, std::size_t* decoded_size);

}  // namespace wfrac_protocol
