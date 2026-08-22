#include "wfrac_protocol/base64.hpp"

#include <limits>

namespace wfrac_protocol {
namespace {

constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int decode_value(uint8_t value) {
  if (value >= static_cast<uint8_t>('A') && value <= static_cast<uint8_t>('Z')) return value - 'A';
  if (value >= static_cast<uint8_t>('a') && value <= static_cast<uint8_t>('z')) return value - 'a' + 26;
  if (value >= static_cast<uint8_t>('0') && value <= static_cast<uint8_t>('9')) return value - '0' + 52;
  if (value == static_cast<uint8_t>('+')) return 62;
  if (value == static_cast<uint8_t>('/')) return 63;
  return -1;
}

}  // namespace

std::size_t base64_encoded_size(std::size_t input_size) {
  const std::size_t max = std::numeric_limits<std::size_t>::max();
  if (input_size > max - 2U) return 0U;
  const std::size_t groups = (input_size + 2U) / 3U;
  if (groups > max / 4U) return 0U;
  return groups * 4U;
}

Base64Error base64_decoded_size(ByteView encoded, std::size_t* decoded_size) {
  if (decoded_size == nullptr) return Base64Error::invalid_argument;
  *decoded_size = 0U;
  if (encoded.size > 0U && encoded.data == nullptr) return Base64Error::invalid_argument;
  if (encoded.size == 0U) return Base64Error::none;
  if ((encoded.size % 4U) != 0U) return Base64Error::invalid_length;

  std::size_t padding = 0U;
  if (encoded.data[encoded.size - 1U] == static_cast<uint8_t>('=')) ++padding;
  if (encoded.size > 1U && encoded.data[encoded.size - 2U] == static_cast<uint8_t>('=')) ++padding;
  if (padding > 2U) return Base64Error::invalid_padding;

  const std::size_t max = std::numeric_limits<std::size_t>::max();
  const std::size_t groups = encoded.size / 4U;
  if (groups > max / 3U) return Base64Error::size_overflow;
  const std::size_t decoded = groups * 3U;
  if (decoded < padding) return Base64Error::invalid_padding;

  for (std::size_t index = 0; index < encoded.size; ++index) {
    const bool is_padding = encoded.data[index] == static_cast<uint8_t>('=');
    if (is_padding) {
      if (index < encoded.size - padding) return Base64Error::invalid_padding;
      continue;
    }
    if (decode_value(encoded.data[index]) < 0) return Base64Error::invalid_character;
  }
  if (padding == 1U && encoded.data[encoded.size - 2U] == static_cast<uint8_t>('=')) {
    return Base64Error::invalid_padding;
  }
  if (padding == 2U && encoded.data[encoded.size - 3U] == static_cast<uint8_t>('=')) {
    return Base64Error::invalid_padding;
  }
  *decoded_size = decoded - padding;
  return Base64Error::none;
}

Base64Error base64_encode(ByteView input, MutableByteView output, std::size_t* encoded_size) {
  if (encoded_size == nullptr) return Base64Error::invalid_argument;
  *encoded_size = 0U;
  if (input.size > 0U && input.data == nullptr) return Base64Error::invalid_argument;
  const std::size_t required = base64_encoded_size(input.size);
  if (input.size > 0U && required == 0U) return Base64Error::size_overflow;
  if (output.size < required) return Base64Error::output_too_small;
  if (required > 0U && output.data == nullptr) return Base64Error::invalid_argument;

  std::size_t input_index = 0U;
  std::size_t output_index = 0U;
  while (input_index < input.size) {
    const std::size_t remaining = input.size - input_index;
    const uint32_t first = input.data[input_index++];
    const uint32_t second = remaining > 1U ? input.data[input_index++] : 0U;
    const uint32_t third = remaining > 2U ? input.data[input_index++] : 0U;
    output.data[output_index++] = static_cast<uint8_t>(kAlphabet[(first >> 2U) & 0x3FU]);
    output.data[output_index++] = static_cast<uint8_t>(kAlphabet[((first & 0x03U) << 4U) | (second >> 4U)]);
    output.data[output_index++] = static_cast<uint8_t>(remaining > 1U ? kAlphabet[((second & 0x0FU) << 2U) | (third >> 6U)] : '=');
    output.data[output_index++] = static_cast<uint8_t>(remaining > 2U ? kAlphabet[third & 0x3FU] : '=');
  }
  *encoded_size = required;
  return Base64Error::none;
}

Base64Error base64_decode(ByteView encoded, MutableByteView output, std::size_t* decoded_size) {
  std::size_t required = 0U;
  const Base64Error validation = base64_decoded_size(encoded, &required);
  if (decoded_size == nullptr) return Base64Error::invalid_argument;
  *decoded_size = 0U;
  if (validation != Base64Error::none) return validation;
  if (output.size < required) return Base64Error::output_too_small;
  if (required > 0U && output.data == nullptr) return Base64Error::invalid_argument;

  std::size_t output_index = 0U;
  for (std::size_t index = 0U; index < encoded.size; index += 4U) {
    const int first = decode_value(encoded.data[index]);
    const int second = decode_value(encoded.data[index + 1U]);
    const int third = encoded.data[index + 2U] == static_cast<uint8_t>('=') ? 0 : decode_value(encoded.data[index + 2U]);
    const int fourth = encoded.data[index + 3U] == static_cast<uint8_t>('=') ? 0 : decode_value(encoded.data[index + 3U]);
    output.data[output_index++] = static_cast<uint8_t>((first << 2) | (second >> 4));
    if (output_index < required) output.data[output_index++] = static_cast<uint8_t>((second << 4) | (third >> 2));
    if (output_index < required) output.data[output_index++] = static_cast<uint8_t>((third << 6) | fourth);
  }
  *decoded_size = required;
  return Base64Error::none;
}

}  // namespace wfrac_protocol
