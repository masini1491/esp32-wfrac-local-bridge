#include "wfrac_api/api_response.hpp"

#include <cstring>
#include <limits>

#include "wfrac_protocol/base64.hpp"

namespace wfrac_api {
namespace {

constexpr int kMaxDepth = 16;

struct Span {
  const uint8_t* begin = nullptr;
  const uint8_t* end = nullptr;
};

bool is_continuation(uint8_t value) {
  return value >= 0x80U && value <= 0xBFU;
}

bool validate_utf8(const uint8_t* input, std::size_t available, std::size_t* sequence_size) {
  if (input == nullptr || available == 0U || sequence_size == nullptr) return false;
  const uint8_t first = input[0];
  if (first <= 0x7FU) {
    *sequence_size = 1U;
    return true;
  }
  if (first >= 0xC2U && first <= 0xDFU) {
    if (available < 2U || !is_continuation(input[1])) return false;
    *sequence_size = 2U;
    return true;
  }
  if (first == 0xE0U) {
    if (available < 3U || input[1] < 0xA0U || input[1] > 0xBFU || !is_continuation(input[2])) return false;
    *sequence_size = 3U;
    return true;
  }
  if (first >= 0xE1U && first <= 0xECU) {
    if (available < 3U || !is_continuation(input[1]) || !is_continuation(input[2])) return false;
    *sequence_size = 3U;
    return true;
  }
  if (first == 0xEDU) {
    if (available < 3U || input[1] < 0x80U || input[1] > 0x9FU || !is_continuation(input[2])) return false;
    *sequence_size = 3U;
    return true;
  }
  if (first >= 0xEEU && first <= 0xEFU) {
    if (available < 3U || !is_continuation(input[1]) || !is_continuation(input[2])) return false;
    *sequence_size = 3U;
    return true;
  }
  if (first == 0xF0U) {
    if (available < 4U || input[1] < 0x90U || input[1] > 0xBFU || !is_continuation(input[2]) ||
        !is_continuation(input[3])) return false;
    *sequence_size = 4U;
    return true;
  }
  if (first >= 0xF1U && first <= 0xF3U) {
    if (available < 4U || !is_continuation(input[1]) || !is_continuation(input[2]) ||
        !is_continuation(input[3])) return false;
    *sequence_size = 4U;
    return true;
  }
  if (first == 0xF4U) {
    if (available < 4U || input[1] < 0x80U || input[1] > 0x8FU || !is_continuation(input[2]) ||
        !is_continuation(input[3])) return false;
    *sequence_size = 4U;
    return true;
  }
  return false;
}

int hex_value(uint8_t value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  return -1;
}

bool read_hex4(const uint8_t** cursor, const uint8_t* end, uint32_t* value) {
  if (cursor == nullptr || *cursor == nullptr || value == nullptr ||
      static_cast<std::size_t>(end - *cursor) < 4U) {
    return false;
  }
  uint32_t result = 0U;
  for (int index = 0; index < 4; ++index) {
    const int digit = hex_value(*(*cursor)++);
    if (digit < 0) return false;
    result = (result << 4U) | static_cast<uint32_t>(digit);
  }
  *value = result;
  return true;
}

std::size_t encode_utf8(uint32_t code_point, uint8_t output[4]) {
  if (code_point < 0x80U) {
    output[0] = static_cast<uint8_t>(code_point);
    return 1U;
  }
  if (code_point < 0x800U) {
    output[0] = static_cast<uint8_t>(0xC0U | (code_point >> 6U));
    output[1] = static_cast<uint8_t>(0x80U | (code_point & 0x3FU));
    return 2U;
  }
  if (code_point < 0x10000U) {
    output[0] = static_cast<uint8_t>(0xE0U | (code_point >> 12U));
    output[1] = static_cast<uint8_t>(0x80U | ((code_point >> 6U) & 0x3FU));
    output[2] = static_cast<uint8_t>(0x80U | (code_point & 0x3FU));
    return 3U;
  }
  output[0] = static_cast<uint8_t>(0xF0U | (code_point >> 18U));
  output[1] = static_cast<uint8_t>(0x80U | ((code_point >> 12U) & 0x3FU));
  output[2] = static_cast<uint8_t>(0x80U | ((code_point >> 6U) & 0x3FU));
  output[3] = static_cast<uint8_t>(0x80U | (code_point & 0x3FU));
  return 4U;
}

ApiResponseError next_decoded_bytes(const uint8_t** cursor, const uint8_t* end, uint8_t output[4],
                                    std::size_t* output_size) {
  if (cursor == nullptr || *cursor == nullptr || output == nullptr || output_size == nullptr || *cursor >= end) {
    return ApiResponseError::malformed_json;
  }
  const uint8_t first = *(*cursor)++;
  if (first >= 0x80U) {
    --(*cursor);
    std::size_t count = 0U;
    if (!validate_utf8(*cursor, static_cast<std::size_t>(end - *cursor), &count)) {
      return ApiResponseError::malformed_json;
    }
    std::memcpy(output, *cursor, count);
    *cursor += count;
    *output_size = count;
    return ApiResponseError::none;
  }
  if (first != '\\') {
    if (first < 0x20U) return ApiResponseError::malformed_json;
    output[0] = first;
    *output_size = 1U;
    return ApiResponseError::none;
  }
  if (*cursor >= end) return ApiResponseError::malformed_json;
  const uint8_t escape = *(*cursor)++;
  if (escape != 'u') {
    switch (escape) {
      case '"': output[0] = '"'; break;
      case '\\': output[0] = '\\'; break;
      case '/': output[0] = '/'; break;
      case 'b': output[0] = '\b'; break;
      case 'f': output[0] = '\f'; break;
      case 'n': output[0] = '\n'; break;
      case 'r': output[0] = '\r'; break;
      case 't': output[0] = '\t'; break;
      default: return ApiResponseError::malformed_json;
    }
    *output_size = 1U;
    return ApiResponseError::none;
  }
  uint32_t code_point = 0U;
  if (!read_hex4(cursor, end, &code_point)) return ApiResponseError::malformed_json;
  if (code_point >= 0xD800U && code_point <= 0xDBFFU) {
    if (static_cast<std::size_t>(end - *cursor) < 6U || (*cursor)[0] != '\\' || (*cursor)[1] != 'u') {
      return ApiResponseError::malformed_json;
    }
    *cursor += 2;
    uint32_t low = 0U;
    if (!read_hex4(cursor, end, &low) || low < 0xDC00U || low > 0xDFFFU) {
      return ApiResponseError::malformed_json;
    }
    code_point = 0x10000U + ((code_point - 0xD800U) << 10U) + (low - 0xDC00U);
  } else if (code_point >= 0xDC00U && code_point <= 0xDFFFU) {
    return ApiResponseError::malformed_json;
  }
  if (code_point == 0U) return ApiResponseError::malformed_json;
  *output_size = encode_utf8(code_point, output);
  return ApiResponseError::none;
}

class Reader {
 public:
  explicit Reader(wfrac_protocol::ByteView input)
      : cursor_(input.data), remaining_(input.data == nullptr ? 0U : input.size) {}

  void skip_whitespace() {
    while (remaining_ != 0U && (*cursor_ == ' ' || *cursor_ == '\n' || *cursor_ == '\r' ||
                                *cursor_ == '\t')) {
      ++cursor_;
      --remaining_;
    }
  }

  bool take(uint8_t expected) {
    skip_whitespace();
    if (remaining_ == 0U || *cursor_ != expected) return false;
    ++cursor_;
    --remaining_;
    return true;
  }

  const uint8_t* position() const { return cursor_; }

  bool done() {
    skip_whitespace();
    return remaining_ == 0U;
  }

  ApiResponseError string(Span* output) {
    skip_whitespace();
    if (remaining_ == 0U || *cursor_ != '"') return ApiResponseError::invalid_field_type;
    ++cursor_;
    --remaining_;
    const uint8_t* begin = cursor_;
    while (remaining_ != 0U) {
      const uint8_t current = *cursor_;
      if (current == '"') {
        *output = {begin, cursor_};
        ++cursor_;
        --remaining_;
        return ApiResponseError::none;
      }
      if (current < 0x20U) return ApiResponseError::malformed_json;
      if (current >= 0x80U) {
        std::size_t count = 0U;
        if (!validate_utf8(cursor_, remaining_, &count)) return ApiResponseError::malformed_json;
        cursor_ += count;
        remaining_ -= count;
        continue;
      }
      ++cursor_;
      --remaining_;
      if (current != '\\') continue;
      if (remaining_ == 0U) return ApiResponseError::malformed_json;
      const uint8_t escape = *cursor_++;
      --remaining_;
      if (escape == 'u') {
        uint32_t high = 0U;
        const uint8_t* code_cursor = cursor_;
        if (!read_hex4(&code_cursor, cursor_ + remaining_, &high)) return ApiResponseError::malformed_json;
        const std::size_t consumed = static_cast<std::size_t>(code_cursor - cursor_);
        cursor_ = code_cursor;
        remaining_ -= consumed;
        if (high >= 0xD800U && high <= 0xDBFFU) {
          if (remaining_ < 2U || cursor_[0] != '\\' || cursor_[1] != 'u') return ApiResponseError::malformed_json;
          cursor_ += 2;
          remaining_ -= 2U;
          uint32_t low = 0U;
          code_cursor = cursor_;
          if (!read_hex4(&code_cursor, cursor_ + remaining_, &low) || low < 0xDC00U || low > 0xDFFFU) {
            return ApiResponseError::malformed_json;
          }
          remaining_ -= static_cast<std::size_t>(code_cursor - cursor_);
          cursor_ = code_cursor;
        } else if (high >= 0xDC00U && high <= 0xDFFFU) {
          return ApiResponseError::malformed_json;
        }
      } else if (escape != '"' && escape != '\\' && escape != '/' && escape != 'b' && escape != 'f' &&
                 escape != 'n' && escape != 'r' && escape != 't') {
        return ApiResponseError::malformed_json;
      }
    }
    return ApiResponseError::malformed_json;
  }

  ApiResponseError integer(int64_t* output) {
    skip_whitespace();
    const uint8_t* start = cursor_;
    const std::size_t before = remaining_;
    const ApiResponseError error = number();
    if (error != ApiResponseError::none) return error;
    const uint8_t* end = start + (before - remaining_);
    const uint8_t* digit = start;
    bool negative = false;
    if (digit != end && *digit == '-') {
      negative = true;
      ++digit;
    }
    uint64_t magnitude = 0U;
    while (digit < end && *digit >= '0' && *digit <= '9') {
      const uint8_t value = static_cast<uint8_t>(*digit++ - '0');
      if (magnitude > (std::numeric_limits<uint64_t>::max() - value) / 10U) {
        return ApiResponseError::integer_overflow;
      }
      magnitude = magnitude * 10U + value;
    }
    if (digit != end) return ApiResponseError::invalid_field_type;
    const uint64_t limit = negative
                               ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1U
                               : static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    if (magnitude > limit) return ApiResponseError::integer_overflow;
    *output = negative
                  ? (magnitude == limit ? std::numeric_limits<int64_t>::min()
                                        : -static_cast<int64_t>(magnitude))
                  : static_cast<int64_t>(magnitude);
    return ApiResponseError::none;
  }

  ApiResponseError value(int depth) {
    skip_whitespace();
    if (depth > kMaxDepth || remaining_ == 0U) {
      return depth > kMaxDepth ? ApiResponseError::nesting_too_deep
                               : ApiResponseError::malformed_json;
    }
    if (*cursor_ == '"') {
      Span ignored{};
      return string(&ignored);
    }
    if (*cursor_ == '{') return object_skip(depth + 1);
    if (*cursor_ == '[') return array_skip(depth + 1);
    if (*cursor_ == '-' || (*cursor_ >= '0' && *cursor_ <= '9')) return number();
    if (match("true") || match("false") || match("null")) return ApiResponseError::none;
    return ApiResponseError::malformed_json;
  }

 private:
  bool match(const char* text) {
    const uint8_t* candidate = cursor_;
    std::size_t available = remaining_;
    while (*text != '\0') {
      if (available == 0U || *candidate++ != static_cast<uint8_t>(*text++)) return false;
      --available;
    }
    cursor_ = candidate;
    remaining_ = available;
    return true;
  }

  ApiResponseError number() {
    skip_whitespace();
    if (remaining_ == 0U) return ApiResponseError::malformed_json;
    if (*cursor_ == '-') {
      ++cursor_;
      --remaining_;
      if (remaining_ == 0U) return ApiResponseError::malformed_json;
    }
    if (*cursor_ == '0') {
      ++cursor_;
      --remaining_;
      if (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') return ApiResponseError::malformed_json;
    } else if (*cursor_ >= '1' && *cursor_ <= '9') {
      while (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') {
        ++cursor_;
        --remaining_;
      }
    } else {
      return ApiResponseError::malformed_json;
    }
    if (remaining_ != 0U && *cursor_ == '.') {
      ++cursor_;
      --remaining_;
      if (remaining_ == 0U || *cursor_ < '0' || *cursor_ > '9') return ApiResponseError::malformed_json;
      while (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') {
        ++cursor_;
        --remaining_;
      }
    }
    if (remaining_ != 0U && (*cursor_ == 'e' || *cursor_ == 'E')) {
      ++cursor_;
      --remaining_;
      if (remaining_ != 0U && (*cursor_ == '+' || *cursor_ == '-')) {
        ++cursor_;
        --remaining_;
      }
      if (remaining_ == 0U || *cursor_ < '0' || *cursor_ > '9') return ApiResponseError::malformed_json;
      while (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') {
        ++cursor_;
        --remaining_;
      }
    }
    return ApiResponseError::none;
  }

  ApiResponseError object_skip(int depth) {
    if (!take('{')) return ApiResponseError::malformed_json;
    if (take('}')) return ApiResponseError::none;
    for (;;) {
      Span key{};
      ApiResponseError error = string(&key);
      if (error != ApiResponseError::none) return error;
      if (!take(':')) return ApiResponseError::malformed_json;
      error = value(depth);
      if (error != ApiResponseError::none) return error;
      if (take('}')) return ApiResponseError::none;
      if (!take(',')) return ApiResponseError::malformed_json;
    }
  }

  ApiResponseError array_skip(int depth) {
    if (!take('[')) return ApiResponseError::malformed_json;
    if (take(']')) return ApiResponseError::none;
    for (;;) {
      const ApiResponseError error = value(depth);
      if (error != ApiResponseError::none) return error;
      if (take(']')) return ApiResponseError::none;
      if (!take(',')) return ApiResponseError::malformed_json;
    }
  }

  const uint8_t* cursor_;
  std::size_t remaining_;
};

bool decoded_equal(Span input, const char* expected) {
  const uint8_t* cursor = input.begin;
  const uint8_t* expected_cursor = reinterpret_cast<const uint8_t*>(expected);
  while (cursor < input.end && *expected_cursor != '\0') {
    uint8_t decoded[4]{};
    std::size_t size = 0U;
    if (next_decoded_bytes(&cursor, input.end, decoded, &size) != ApiResponseError::none) return false;
    for (std::size_t index = 0U; index < size; ++index) {
      if (*expected_cursor == '\0' || decoded[index] != *expected_cursor++) return false;
    }
  }
  return cursor == input.end && *expected_cursor == '\0';
}

ApiResponseError decode(Span input, wfrac_protocol::MutableByteView output, std::size_t* size) {
  if (size != nullptr) *size = 0U;
  if (output.size != 0U && output.data != nullptr) output.data[0] = 0U;
  std::size_t written = 0U;
  const uint8_t* cursor = input.begin;
  while (cursor < input.end) {
    uint8_t decoded[4]{};
    std::size_t count = 0U;
    const ApiResponseError error = next_decoded_bytes(&cursor, input.end, decoded, &count);
    if (error != ApiResponseError::none) return error;
    if (output.data == nullptr || written > output.size || output.size - written <= count) {
      return ApiResponseError::output_too_small;
    }
    std::memcpy(output.data + written, decoded, count);
    written += count;
  }
  if (written == 0U) return ApiResponseError::missing_required_field;
  if (output.data == nullptr || output.size <= written) return ApiResponseError::output_too_small;
  output.data[written] = 0U;
  if (size != nullptr) *size = written;
  return ApiResponseError::none;
}

ApiResponseError base64_error(wfrac_protocol::Base64Error error) {
  switch (error) {
    case wfrac_protocol::Base64Error::none: return ApiResponseError::none;
    case wfrac_protocol::Base64Error::output_too_small: return ApiResponseError::output_too_small;
    case wfrac_protocol::Base64Error::invalid_argument: return ApiResponseError::invalid_argument;
    case wfrac_protocol::Base64Error::size_overflow: return ApiResponseError::integer_overflow;
    case wfrac_protocol::Base64Error::invalid_length:
    case wfrac_protocol::Base64Error::invalid_character:
    case wfrac_protocol::Base64Error::invalid_padding: return ApiResponseError::invalid_base64;
  }
  return ApiResponseError::invalid_base64;
}

}  // namespace

ApiResponseError parse_api_response_envelope(wfrac_protocol::ByteView input, const char* expected_command,
                                              ApiResponseEnvelopeView* output) {
  if (output != nullptr) *output = {};
  if (output == nullptr || expected_command == nullptr || *expected_command == '\0' ||
      (input.size != 0U && input.data == nullptr)) {
    return ApiResponseError::invalid_argument;
  }
  ApiResponseEnvelopeView parsed{};
  Reader reader(input);
  if (!reader.take('{')) return ApiResponseError::malformed_json;
  bool has_command = false;
  bool has_result = false;
  bool has_contents = false;
  Span command{};
  Span contents{};
  if (!reader.take('}')) {
    for (;;) {
      Span key{};
      ApiResponseError error = reader.string(&key);
      if (error != ApiResponseError::none) return error;
      if (!reader.take(':')) return ApiResponseError::malformed_json;
      if (decoded_equal(key, "command")) {
        if (has_command) return ApiResponseError::malformed_json;
        error = reader.string(&command);
        if (error != ApiResponseError::none) return error;
        has_command = true;
      } else if (decoded_equal(key, "result")) {
        if (has_result) return ApiResponseError::malformed_json;
        error = reader.integer(&parsed.result);
        if (error != ApiResponseError::none) return error;
        has_result = true;
      } else if (decoded_equal(key, "contents")) {
        if (has_contents) return ApiResponseError::malformed_json;
        reader.skip_whitespace();
        const uint8_t* begin = reader.position();
        error = reader.value(0);
        if (error != ApiResponseError::none) return error;
        contents = {begin, reader.position()};
        has_contents = true;
      } else {
        error = reader.value(0);
        if (error != ApiResponseError::none) return error;
      }
      if (reader.take('}')) break;
      if (!reader.take(',')) return ApiResponseError::malformed_json;
    }
  }
  if (!reader.done()) return ApiResponseError::malformed_json;
  if (!has_command || !has_result) return ApiResponseError::missing_required_field;
  if (!decoded_equal(command, expected_command)) return ApiResponseError::unexpected_command;
  parsed.has_contents = has_contents;
  parsed.contents_json = has_contents
                             ? wfrac_protocol::ByteView{contents.begin,
                                                        static_cast<std::size_t>(contents.end - contents.begin)}
                             : wfrac_protocol::ByteView{};
  *output = parsed;
  return ApiResponseError::none;
}

ApiResponseError parse_get_device_info_contents(wfrac_protocol::ByteView input,
                                                 wfrac_protocol::MutableByteView aircon_id_output,
                                                 wfrac_protocol::MutableByteView mac_address_output,
                                                 GetDeviceInfoContents* output) {
  if (output != nullptr) *output = {};
  if (output == nullptr || (input.size != 0U && input.data == nullptr)) return ApiResponseError::invalid_argument;
  if (aircon_id_output.size != 0U && aircon_id_output.data != nullptr) aircon_id_output.data[0] = 0U;
  if (mac_address_output.size != 0U && mac_address_output.data != nullptr) mac_address_output.data[0] = 0U;
  const auto fail = [&](ApiResponseError error) {
    if (aircon_id_output.size != 0U && aircon_id_output.data != nullptr) aircon_id_output.data[0] = 0U;
    if (mac_address_output.size != 0U && mac_address_output.data != nullptr) mac_address_output.data[0] = 0U;
    *output = {};
    return error;
  };
  GetDeviceInfoContents parsed{};
  Reader reader(input);
  if (!reader.take('{')) return fail(ApiResponseError::invalid_field_type);
  bool has_aircon_id = false;
  bool has_mac_address = false;
  bool has_ap_mode = false;
  if (!reader.take('}')) {
    for (;;) {
      Span key{};
      Span value{};
      ApiResponseError error = reader.string(&key);
      if (error != ApiResponseError::none) return fail(error);
      if (!reader.take(':')) return fail(ApiResponseError::malformed_json);
      if (decoded_equal(key, "airconId")) {
        if (has_aircon_id) return fail(ApiResponseError::malformed_json);
        error = reader.string(&value);
        if (error != ApiResponseError::none) return fail(error);
        error = decode(value, aircon_id_output, &parsed.aircon_id_size);
        if (error != ApiResponseError::none) return fail(error);
        has_aircon_id = true;
      } else if (decoded_equal(key, "macAddress")) {
        if (has_mac_address) return fail(ApiResponseError::malformed_json);
        error = reader.string(&value);
        if (error != ApiResponseError::none) return fail(error);
        error = decode(value, mac_address_output, &parsed.mac_address_size);
        if (error != ApiResponseError::none) return fail(error);
        parsed.has_mac_address = true;
        has_mac_address = true;
      } else if (decoded_equal(key, "apMode")) {
        if (has_ap_mode) return fail(ApiResponseError::malformed_json);
        error = reader.integer(&parsed.ap_mode);
        if (error != ApiResponseError::none) return fail(error);
        parsed.has_ap_mode = true;
        has_ap_mode = true;
      } else {
        error = reader.value(0);
        if (error != ApiResponseError::none) return fail(error);
      }
      if (reader.take('}')) break;
      if (!reader.take(',')) return fail(ApiResponseError::malformed_json);
    }
  }
  if (!reader.done()) return fail(ApiResponseError::malformed_json);
  if (!has_aircon_id) return fail(ApiResponseError::missing_required_field);
  *output = parsed;
  return ApiResponseError::none;
}

ApiResponseError parse_get_aircon_stat_contents(
    wfrac_protocol::ByteView input, wfrac_protocol::MutableByteView aircon_id_output,
    wfrac_protocol::MutableByteView base64_scratch, wfrac_protocol::MutableByteView raw_aircon_stat_output,
    GetAirconStatContents* output) {
  if (output != nullptr) *output = {};
  if (output == nullptr || (input.size != 0U && input.data == nullptr)) return ApiResponseError::invalid_argument;
  if (aircon_id_output.size != 0U && aircon_id_output.data != nullptr) aircon_id_output.data[0] = 0U;
  if (base64_scratch.size != 0U && base64_scratch.data != nullptr) base64_scratch.data[0] = 0U;
  const auto fail = [&](ApiResponseError error) {
    if (aircon_id_output.size != 0U && aircon_id_output.data != nullptr) aircon_id_output.data[0] = 0U;
    *output = {};
    return error;
  };
  Reader reader(input);
  if (!reader.take('{')) return fail(ApiResponseError::invalid_field_type);
  bool has_aircon_stat = false;
  bool has_aircon_id = false;
  Span aircon_stat{};
  Span aircon_id{};
  if (!reader.take('}')) {
    for (;;) {
      Span key{};
      ApiResponseError error = reader.string(&key);
      if (error != ApiResponseError::none) return fail(error);
      if (!reader.take(':')) return fail(ApiResponseError::malformed_json);
      if (decoded_equal(key, "airconStat")) {
        if (has_aircon_stat) return fail(ApiResponseError::malformed_json);
        error = reader.string(&aircon_stat);
        if (error != ApiResponseError::none) return fail(error);
        has_aircon_stat = true;
      } else if (decoded_equal(key, "airconId")) {
        if (has_aircon_id) return fail(ApiResponseError::malformed_json);
        error = reader.string(&aircon_id);
        if (error != ApiResponseError::none) return fail(error);
        has_aircon_id = true;
      } else {
        error = reader.value(0);
        if (error != ApiResponseError::none) return fail(error);
      }
      if (reader.take('}')) break;
      if (!reader.take(',')) return fail(ApiResponseError::malformed_json);
    }
  }
  if (!reader.done()) return fail(ApiResponseError::malformed_json);
  if (!has_aircon_stat) return fail(ApiResponseError::missing_required_field);
  GetAirconStatContents parsed{};
  if (has_aircon_id) {
    const ApiResponseError error = decode(aircon_id, aircon_id_output, &parsed.aircon_id_size);
    if (error != ApiResponseError::none) return fail(error);
    parsed.has_aircon_id = true;
  }
  const ApiResponseError decoded = decode(aircon_stat, base64_scratch, &parsed.encoded_aircon_stat_size);
  if (decoded != ApiResponseError::none) return fail(decoded);
  std::size_t raw_size = 0U;
  ApiResponseError error = base64_error(wfrac_protocol::base64_decoded_size(
      {base64_scratch.data, parsed.encoded_aircon_stat_size}, &raw_size));
  if (error != ApiResponseError::none) return fail(error);
  if (raw_aircon_stat_output.size < raw_size) return fail(ApiResponseError::output_too_small);
  error = base64_error(wfrac_protocol::base64_decode(
      {base64_scratch.data, parsed.encoded_aircon_stat_size}, raw_aircon_stat_output, &raw_size));
  if (error != ApiResponseError::none) return fail(error);
  wfrac_protocol::AirconFrameView frame{};
  if (wfrac_protocol::parse_aircon_stat({raw_aircon_stat_output.data, raw_size}, &frame) !=
      wfrac_protocol::FrameError::none) {
    return fail(ApiResponseError::invalid_aircon_stat);
  }
  if (!frame.command.crc_valid || !frame.receive.crc_valid) return fail(ApiResponseError::crc_mismatch);
  parsed.raw_aircon_stat_size = raw_size;
  parsed.frame = frame;
  *output = parsed;
  return ApiResponseError::none;
}

}  // namespace wfrac_api
