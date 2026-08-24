#include "wfrac_api/api_response.hpp"

#include <cstring>
#include <limits>

#include "wfrac_protocol/base64.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

namespace wfrac_api {
namespace {
constexpr int kMaxDepth = 16;
struct Span { const uint8_t* begin = nullptr; const uint8_t* end = nullptr; };

class Reader {
 public:
  explicit Reader(wfrac_protocol::ByteView input)
      : cursor_(input.data), remaining_(input.data == nullptr ? 0U : input.size) {}
  void ws() { while (remaining_ != 0U && (*cursor_ == ' ' || *cursor_ == '\n' || *cursor_ == '\r' || *cursor_ == '\t')) { ++cursor_; --remaining_; } }
  bool at(uint8_t c) { ws(); return remaining_ != 0U && *cursor_ == c; }
  bool take(uint8_t c) { if (!at(c)) return false; ++cursor_; --remaining_; return true; }
  const uint8_t* pos() const { return cursor_; }
  bool done() { ws(); return remaining_ == 0U; }
  ApiResponseError string(Span* out) {
    ws(); if (remaining_ == 0U || *cursor_++ != '"') return ApiResponseError::invalid_field_type; --remaining_;
    const uint8_t* begin = cursor_;
    while (remaining_ != 0U) {
      const uint8_t c = *cursor_++; --remaining_;
      if (c == '"') { *out = {begin, cursor_ - 1}; return ApiResponseError::none; }
      if (c < 0x20U) return ApiResponseError::malformed_json;
      if (c != '\\') continue;
      if (remaining_ == 0U) return ApiResponseError::malformed_json;
      const uint8_t escape = *cursor_++; --remaining_;
      if (escape == 'u') {
        const int high = hex4(); if (high < 0) return ApiResponseError::malformed_json;
        if (high >= 0xD800 && high <= 0xDBFF) {
          if (remaining_ < 2U || cursor_[0] != '\\' || cursor_[1] != 'u') return ApiResponseError::malformed_json;
          cursor_ += 2; remaining_ -= 2U;
          const int low = hex4(); if (low < 0xDC00 || low > 0xDFFF) return ApiResponseError::malformed_json;
        } else if (high >= 0xDC00 && high <= 0xDFFF) return ApiResponseError::malformed_json;
      } else if (escape != '"' && escape != '\\' && escape != '/' && escape != 'b' && escape != 'f' && escape != 'n' && escape != 'r' && escape != 't') {
        return ApiResponseError::malformed_json;
      }
    }
    return ApiResponseError::malformed_json;
  }
  ApiResponseError integer(int64_t* value) {
    ws();
    const uint8_t* start = cursor_; const std::size_t before = remaining_;
    const ApiResponseError error = number(); if (error != ApiResponseError::none) return error;
    const uint8_t* end = start + (before - remaining_); const uint8_t* digit = start; bool negative = false;
    if (digit != end && *digit == '-') { negative = true; ++digit; }
    uint64_t magnitude = 0U;
    while (digit < end && *digit >= '0' && *digit <= '9') { const uint8_t d = static_cast<uint8_t>(*digit++ - '0'); if (magnitude > (std::numeric_limits<uint64_t>::max() - d) / 10U) return ApiResponseError::integer_overflow; magnitude = magnitude * 10U + d; }
    if (digit != end) return ApiResponseError::invalid_field_type;
    const uint64_t limit = negative ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1U : static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    if (magnitude > limit) return ApiResponseError::integer_overflow;
    *value = negative ? (magnitude == limit ? std::numeric_limits<int64_t>::min() : -static_cast<int64_t>(magnitude)) : static_cast<int64_t>(magnitude);
    return ApiResponseError::none;
  }
  ApiResponseError value(int depth) {
    ws(); if (depth > kMaxDepth || remaining_ == 0U) return depth > kMaxDepth ? ApiResponseError::nesting_too_deep : ApiResponseError::malformed_json;
    if (*cursor_ == '"') { Span ignored{}; return string(&ignored); }
    if (*cursor_ == '{') return object_skip(depth + 1); if (*cursor_ == '[') return array_skip(depth + 1);
    if (*cursor_ == '-' || (*cursor_ >= '0' && *cursor_ <= '9')) return number();
    if (match("true") || match("false") || match("null")) return ApiResponseError::none;
    return ApiResponseError::malformed_json;
  }
 private:
  int hex4() { if (remaining_ < 4U) return -1; int result = 0; for (int i = 0; i < 4; ++i) { const uint8_t c = *cursor_++; --remaining_; const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; if (d < 0) return -1; result = (result << 4) | d; } return result; }
  bool match(const char* text) { const uint8_t* p = cursor_; std::size_t n = remaining_; while (*text != '\0') { if (n == 0U || *p++ != static_cast<uint8_t>(*text++)) return false; --n; } cursor_ = p; remaining_ = n; return true; }
  ApiResponseError number() {
    ws(); if (remaining_ == 0U) return ApiResponseError::malformed_json;
    if (*cursor_ == '-') { ++cursor_; --remaining_; if (remaining_ == 0U) return ApiResponseError::malformed_json; }
    if (*cursor_ == '0') { ++cursor_; --remaining_; if (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') return ApiResponseError::malformed_json; }
    else if (*cursor_ >= '1' && *cursor_ <= '9') { while (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') { ++cursor_; --remaining_; } }
    else return ApiResponseError::malformed_json;
    if (remaining_ != 0U && *cursor_ == '.') { ++cursor_; --remaining_; if (remaining_ == 0U || *cursor_ < '0' || *cursor_ > '9') return ApiResponseError::malformed_json; while (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') { ++cursor_; --remaining_; } }
    if (remaining_ != 0U && (*cursor_ == 'e' || *cursor_ == 'E')) { ++cursor_; --remaining_; if (remaining_ != 0U && (*cursor_ == '+' || *cursor_ == '-')) { ++cursor_; --remaining_; } if (remaining_ == 0U || *cursor_ < '0' || *cursor_ > '9') return ApiResponseError::malformed_json; while (remaining_ != 0U && *cursor_ >= '0' && *cursor_ <= '9') { ++cursor_; --remaining_; } }
    return ApiResponseError::none;
  }
  ApiResponseError object_skip(int depth) { if (!take('{')) return ApiResponseError::malformed_json; if (take('}')) return ApiResponseError::none; for (;;) { Span key{}; ApiResponseError e = string(&key); if (e != ApiResponseError::none) return e; if (!take(':')) return ApiResponseError::malformed_json; e = value(depth); if (e != ApiResponseError::none) return e; if (take('}')) return ApiResponseError::none; if (!take(',')) return ApiResponseError::malformed_json; } }
  ApiResponseError array_skip(int depth) { if (!take('[')) return ApiResponseError::malformed_json; if (take(']')) return ApiResponseError::none; for (;;) { const ApiResponseError e = value(depth); if (e != ApiResponseError::none) return e; if (take(']')) return ApiResponseError::none; if (!take(',')) return ApiResponseError::malformed_json; } }
  const uint8_t* cursor_; std::size_t remaining_;
};

bool equal(Span span, const char* text) { const std::size_t n = std::strlen(text); return static_cast<std::size_t>(span.end - span.begin) == n && std::memcmp(span.begin, text, n) == 0; }

ApiResponseError decode(Span input, wfrac_protocol::MutableByteView output, std::size_t* size) {
  if (size != nullptr) *size = 0U; if (output.size != 0U && output.data != nullptr) output.data[0] = 0U; std::size_t written = 0U;
  for (const uint8_t* p = input.begin; p < input.end;) {
    uint32_t cp = *p++; if (cp == '\\') { if (p >= input.end) return ApiResponseError::malformed_json; const uint8_t e = *p++; if (e == 'u') { auto hex = [](uint8_t c) -> int { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }; if (static_cast<std::size_t>(input.end - p) < 4U) return ApiResponseError::malformed_json; cp = 0U; for (int i = 0; i < 4; ++i) { const int d = hex(*p++); if (d < 0) return ApiResponseError::malformed_json; cp = (cp << 4U) | static_cast<uint32_t>(d); } if (cp >= 0xD800U && cp <= 0xDBFFU) { if (static_cast<std::size_t>(input.end - p) < 6U || p[0] != '\\' || p[1] != 'u') return ApiResponseError::malformed_json; p += 2; uint32_t low = 0U; for (int i = 0; i < 4; ++i) { const int d = hex(*p++); if (d < 0) return ApiResponseError::malformed_json; low = (low << 4U) | static_cast<uint32_t>(d); } if (low < 0xDC00U || low > 0xDFFFU) return ApiResponseError::malformed_json; cp = 0x10000U + ((cp - 0xD800U) << 10U) + low - 0xDC00U; } else if (cp >= 0xDC00U && cp <= 0xDFFFU) return ApiResponseError::malformed_json; } else { switch (e) { case '"': cp = '"'; break; case '\\': cp = '\\'; break; case '/': cp = '/'; break; case 'b': cp = '\b'; break; case 'f': cp = '\f'; break; case 'n': cp = '\n'; break; case 'r': cp = '\r'; break; case 't': cp = '\t'; break; default: return ApiResponseError::malformed_json; } } }
    if (cp == 0U) return ApiResponseError::malformed_json; uint8_t bytes[4]{}; const std::size_t n = cp < 0x80U ? 1U : cp < 0x800U ? 2U : cp < 0x10000U ? 3U : 4U; if (n == 1U) bytes[0] = static_cast<uint8_t>(cp); else if (n == 2U) { bytes[0] = static_cast<uint8_t>(0xC0U | (cp >> 6U)); bytes[1] = static_cast<uint8_t>(0x80U | (cp & 63U)); } else if (n == 3U) { bytes[0] = static_cast<uint8_t>(0xE0U | (cp >> 12U)); bytes[1] = static_cast<uint8_t>(0x80U | ((cp >> 6U) & 63U)); bytes[2] = static_cast<uint8_t>(0x80U | (cp & 63U)); } else { bytes[0] = static_cast<uint8_t>(0xF0U | (cp >> 18U)); bytes[1] = static_cast<uint8_t>(0x80U | ((cp >> 12U) & 63U)); bytes[2] = static_cast<uint8_t>(0x80U | ((cp >> 6U) & 63U)); bytes[3] = static_cast<uint8_t>(0x80U | (cp & 63U)); }
    if (output.data == nullptr || written > output.size || output.size - written <= n) return ApiResponseError::output_too_small; std::memcpy(output.data + written, bytes, n); written += n;
  }
  if (written == 0U) return ApiResponseError::missing_required_field; if (output.data == nullptr || output.size <= written) return ApiResponseError::output_too_small; output.data[written] = 0U; if (size != nullptr) *size = written; return ApiResponseError::none;
}

}  // namespace

ApiResponseError parse_api_response_envelope(wfrac_protocol::ByteView input, const char* expected, ApiResponseEnvelopeView* output) {
  if (output != nullptr) *output = {}; if (output == nullptr || expected == nullptr || *expected == '\0' || (input.size != 0U && input.data == nullptr)) return ApiResponseError::invalid_argument; ApiResponseEnvelopeView parsed{}; Reader r(input); if (!r.take('{')) return ApiResponseError::malformed_json; bool command = false, result = false, contents = false; Span command_span{}, content_span{};
  if (!r.take('}')) for (;;) { Span key{}; ApiResponseError e = r.string(&key); if (e != ApiResponseError::none) return e; if (!r.take(':')) return ApiResponseError::malformed_json; if (equal(key, "command")) { if (command) return ApiResponseError::malformed_json; e = r.string(&command_span); if (e != ApiResponseError::none) return e; command = true; } else if (equal(key, "result")) { if (result) return ApiResponseError::malformed_json; e = r.integer(&parsed.result); if (e != ApiResponseError::none) return e; result = true; } else if (equal(key, "contents")) { if (contents) return ApiResponseError::malformed_json; r.ws(); const uint8_t* begin = r.pos(); e = r.value(0); if (e != ApiResponseError::none) return e; content_span = {begin, r.pos()}; contents = true; } else { e = r.value(0); if (e != ApiResponseError::none) return e; } if (r.take('}')) break; if (!r.take(',')) return ApiResponseError::malformed_json; }
  if (!r.done()) return ApiResponseError::malformed_json; if (!command || !result) return ApiResponseError::missing_required_field; if (!equal(command_span, expected)) return ApiResponseError::unexpected_command; parsed.has_contents = contents; parsed.contents_json = contents ? wfrac_protocol::ByteView{content_span.begin, static_cast<std::size_t>(content_span.end - content_span.begin)} : wfrac_protocol::ByteView{}; *output = parsed; return ApiResponseError::none;
}

ApiResponseError parse_get_device_info_contents(wfrac_protocol::ByteView input, wfrac_protocol::MutableByteView air, wfrac_protocol::MutableByteView mac, GetDeviceInfoContents* output) {
  if (output != nullptr) *output = {}; if (output == nullptr || (input.size != 0U && input.data == nullptr)) return ApiResponseError::invalid_argument; if (air.size != 0U && air.data != nullptr) air.data[0] = 0U; if (mac.size != 0U && mac.data != nullptr) mac.data[0] = 0U; GetDeviceInfoContents parsed{}; Reader r(input); if (!r.take('{')) return ApiResponseError::invalid_field_type; bool id = false, has_mac = false, has_ap = false;
  if (!r.take('}')) for (;;) { Span key{}, value{}; ApiResponseError e = r.string(&key); if (e != ApiResponseError::none) return e; if (!r.take(':')) return ApiResponseError::malformed_json; if (equal(key, "airconId")) { if (id) return ApiResponseError::malformed_json; e = r.string(&value); if (e != ApiResponseError::none) return e; e = decode(value, air, &parsed.aircon_id_size); if (e != ApiResponseError::none) return e; id = true; } else if (equal(key, "macAddress")) { if (has_mac) return ApiResponseError::malformed_json; e = r.string(&value); if (e != ApiResponseError::none) return e; e = decode(value, mac, &parsed.mac_address_size); if (e != ApiResponseError::none) return e; parsed.has_mac_address = true; has_mac = true; } else if (equal(key, "apMode")) { if (has_ap) return ApiResponseError::malformed_json; e = r.integer(&parsed.ap_mode); if (e != ApiResponseError::none) return e; parsed.has_ap_mode = true; has_ap = true; } else { e = r.value(0); if (e != ApiResponseError::none) return e; } if (r.take('}')) break; if (!r.take(',')) return ApiResponseError::malformed_json; }
  if (!r.done()) return ApiResponseError::malformed_json; if (!id) return ApiResponseError::missing_required_field; *output = parsed; return ApiResponseError::none;
}

static ApiResponseError base64_error(wfrac_protocol::Base64Error error) { switch (error) { case wfrac_protocol::Base64Error::none: return ApiResponseError::none; case wfrac_protocol::Base64Error::output_too_small: return ApiResponseError::output_too_small; case wfrac_protocol::Base64Error::invalid_argument: return ApiResponseError::invalid_argument; case wfrac_protocol::Base64Error::size_overflow: return ApiResponseError::integer_overflow; case wfrac_protocol::Base64Error::invalid_length: case wfrac_protocol::Base64Error::invalid_character: case wfrac_protocol::Base64Error::invalid_padding: return ApiResponseError::invalid_base64; } return ApiResponseError::invalid_base64; }

ApiResponseError parse_get_aircon_stat_contents(wfrac_protocol::ByteView input, wfrac_protocol::MutableByteView air, wfrac_protocol::MutableByteView scratch, wfrac_protocol::MutableByteView raw, GetAirconStatContents* output) {
  if (output != nullptr) *output = {}; if (output == nullptr || (input.size != 0U && input.data == nullptr)) return ApiResponseError::invalid_argument; if (air.size != 0U && air.data != nullptr) air.data[0] = 0U; if (scratch.size != 0U && scratch.data != nullptr) scratch.data[0] = 0U; Reader r(input); if (!r.take('{')) return ApiResponseError::invalid_field_type; bool stat = false, id_present = false; Span stat_span{}, id_span{};
  if (!r.take('}')) for (;;) { Span key{}; ApiResponseError e = r.string(&key); if (e != ApiResponseError::none) return e; if (!r.take(':')) return ApiResponseError::malformed_json; if (equal(key, "airconStat")) { if (stat) return ApiResponseError::malformed_json; e = r.string(&stat_span); if (e != ApiResponseError::none) return e; stat = true; } else if (equal(key, "airconId")) { if (id_present) return ApiResponseError::malformed_json; e = r.string(&id_span); if (e != ApiResponseError::none) return e; id_present = true; } else { e = r.value(0); if (e != ApiResponseError::none) return e; } if (r.take('}')) break; if (!r.take(',')) return ApiResponseError::malformed_json; }
  if (!r.done()) return ApiResponseError::malformed_json; if (!stat) return ApiResponseError::missing_required_field; GetAirconStatContents parsed{}; if (id_present) { const ApiResponseError e = decode(id_span, air, &parsed.aircon_id_size); if (e != ApiResponseError::none) return e; parsed.has_aircon_id = true; } const ApiResponseError e = decode(stat_span, scratch, &parsed.encoded_aircon_stat_size); if (e != ApiResponseError::none) return e; std::size_t raw_size = 0U; ApiResponseError mapped = base64_error(wfrac_protocol::base64_decoded_size({scratch.data, parsed.encoded_aircon_stat_size}, &raw_size)); if (mapped != ApiResponseError::none) return mapped; if (raw.size < raw_size) return ApiResponseError::output_too_small; mapped = base64_error(wfrac_protocol::base64_decode({scratch.data, parsed.encoded_aircon_stat_size}, raw, &raw_size)); if (mapped != ApiResponseError::none) return mapped; wfrac_protocol::AirconFrameView frame{}; if (wfrac_protocol::parse_aircon_stat({raw.data, raw_size}, &frame) != wfrac_protocol::FrameError::none) return ApiResponseError::invalid_aircon_stat; if (!frame.command.crc_valid || !frame.receive.crc_valid) return ApiResponseError::crc_mismatch; parsed.raw_aircon_stat_size = raw_size; parsed.frame = frame; *output = parsed; return ApiResponseError::none;
}
}  // namespace wfrac_api
