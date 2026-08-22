#include "wfrac_api/api_request.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

#include "wfrac_protocol/aircon_stat.hpp"
#include "wfrac_protocol/base64.hpp"

namespace wfrac_api {
namespace {

constexpr char kGetDeviceInfoPath[] = "/beaver/command/getDeviceInfo";
constexpr char kGetAirconStatPath[] = "/beaver/command/getAirconStat";
constexpr char kSetAirconStatPath[] = "/beaver/command/setAirconStat";

enum class RequestKind { get_device_info, get_aircon_stat, set_aircon_stat };

bool non_empty(const char* value) { return value != nullptr && value[0] != '\0'; }

void reset_request(ApiRequestView* request) {
  if (request != nullptr) *request = {};
}

bool checked_add(std::size_t left, std::size_t right, std::size_t* output) {
  if (left > std::numeric_limits<std::size_t>::max() - right) return false;
  *output = left + right;
  return true;
}

bool append_size(std::size_t amount, std::size_t* total) {
  std::size_t next = 0;
  if (!checked_add(*total, amount, &next)) return false;
  *total = next;
  return true;
}

bool escaped_size(const char* value, std::size_t* size) {
  std::size_t total = 0;
  for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(value); *cursor != 0U; ++cursor) {
    const std::size_t encoded = (*cursor == '"' || *cursor == '\\' || *cursor == '\b' || *cursor == '\f' ||
                                 *cursor == '\n' || *cursor == '\r' || *cursor == '\t')
                                    ? 2U
                                    : (*cursor < 0x20U ? 6U : 1U);
    if (!append_size(encoded, &total)) return false;
  }
  *size = total;
  return true;
}

std::size_t positive_decimal_size(int64_t value) {
  std::size_t count = 1U;
  while (value >= 10) {
    value /= 10;
    ++count;
  }
  return count;
}

bool append_literal_size(const char* literal, std::size_t* total) {
  std::size_t length = 0;
  while (literal[length] != '\0') ++length;
  return append_size(length, total);
}

bool calculate_size(RequestKind kind, const ApiClientContext& context, const char* aircon_id, int64_t timestamp,
                    std::size_t base64_size, std::size_t* total) {
  std::size_t api_version_size = 0;
  std::size_t device_id_size = 0;
  std::size_t operator_id_size = 0;
  std::size_t aircon_id_size = 0;
  if (!escaped_size(context.api_version, &api_version_size) || !escaped_size(context.device_id, &device_id_size) ||
      !escaped_size(context.operator_id, &operator_id_size) ||
      (aircon_id != nullptr && !escaped_size(aircon_id, &aircon_id_size))) {
    return false;
  }

  std::size_t size = 0;
  const char* command = kind == RequestKind::get_device_info
                            ? "getDeviceInfo"
                            : (kind == RequestKind::get_aircon_stat ? "getAirconStat" : "setAirconStat");
  if (!append_literal_size("{\"apiVer\":\"", &size) || !append_size(api_version_size, &size) ||
      !append_literal_size("\",\"command\":\"", &size) || !append_literal_size(command, &size) ||
      !append_literal_size("\",\"deviceId\":\"", &size) || !append_size(device_id_size, &size) ||
      !append_literal_size("\",\"operatorId\":\"", &size) || !append_size(operator_id_size, &size) ||
      !append_literal_size("\",\"timestamp\":", &size) || !append_size(positive_decimal_size(timestamp), &size)) {
    return false;
  }
  if (kind == RequestKind::get_device_info) {
    if (!append_literal_size("}", &size)) return false;
  } else if (kind == RequestKind::get_aircon_stat) {
    if (!append_literal_size(",\"contents\":{\"airconId\":\"", &size) || !append_size(aircon_id_size, &size) ||
        !append_literal_size("\"}}", &size)) {
      return false;
    }
  } else if (!append_literal_size(",\"contents\":{\"airconId\":\"", &size) || !append_size(aircon_id_size, &size) ||
             !append_literal_size("\",\"airconStat\":\"", &size) || !append_size(base64_size, &size) ||
             !append_literal_size("\"}}", &size)) {
    return false;
  }
  *total = size;
  return true;
}

void append_literal(const char* literal, uint8_t* output, std::size_t* cursor) {
  while (*literal != '\0') output[(*cursor)++] = static_cast<uint8_t>(*literal++);
}

void append_escaped(const char* value, uint8_t* output, std::size_t* cursor) {
  constexpr char kHex[] = "0123456789ABCDEF";
  for (const unsigned char* input = reinterpret_cast<const unsigned char*>(value); *input != 0U; ++input) {
    switch (*input) {
      case '"': append_literal("\\\"", output, cursor); break;
      case '\\': append_literal("\\\\", output, cursor); break;
      case '\b': append_literal("\\b", output, cursor); break;
      case '\f': append_literal("\\f", output, cursor); break;
      case '\n': append_literal("\\n", output, cursor); break;
      case '\r': append_literal("\\r", output, cursor); break;
      case '\t': append_literal("\\t", output, cursor); break;
      default:
        if (*input < 0x20U) {
          output[(*cursor)++] = '\\';
          output[(*cursor)++] = 'u';
          output[(*cursor)++] = '0';
          output[(*cursor)++] = '0';
          output[(*cursor)++] = static_cast<uint8_t>(kHex[*input >> 4U]);
          output[(*cursor)++] = static_cast<uint8_t>(kHex[*input & 0x0FU]);
        } else {
          output[(*cursor)++] = *input;
        }
        break;
    }
  }
}

void append_positive_decimal(int64_t value, uint8_t* output, std::size_t* cursor) {
  char digits[20]{};
  std::size_t count = 0;
  do {
    digits[count++] = static_cast<char>('0' + (value % 10));
    value /= 10;
  } while (value != 0);
  while (count != 0U) output[(*cursor)++] = static_cast<uint8_t>(digits[--count]);
}

ApiRequestError validate_context(const ApiClientContext& context, ITimeProvider* time_provider,
                                 wfrac_protocol::MutableByteView output, ApiRequestView* request) {
  if (request == nullptr || time_provider == nullptr || output.data == nullptr || !non_empty(context.api_version) ||
      !non_empty(context.device_id) || !non_empty(context.operator_id)) {
    return ApiRequestError::invalid_argument;
  }
  return ApiRequestError::none;
}

ApiRequestError validate_time(ITimeProvider* provider, TimeSample* sample) {
  *sample = provider->now();
  if (sample->quality == TimeQuality::unavailable) return ApiRequestError::time_unavailable;
  if (sample->unix_seconds <= 0) return ApiRequestError::invalid_timestamp;
  return ApiRequestError::none;
}

ApiRequestError build_request(RequestKind kind, const ApiClientContext& context, const char* aircon_id,
                              wfrac_protocol::ByteView raw_aircon_stat, ITimeProvider* time_provider,
                              wfrac_protocol::MutableByteView output, ApiRequestView* request) {
  reset_request(request);
  const ApiRequestError context_error = validate_context(context, time_provider, output, request);
  if (context_error != ApiRequestError::none) return context_error;
  if (kind != RequestKind::get_device_info && !non_empty(aircon_id)) return ApiRequestError::invalid_argument;

  std::size_t base64_size = 0;
  if (kind == RequestKind::set_aircon_stat) {
    if (raw_aircon_stat.data == nullptr || raw_aircon_stat.size == 0U) return ApiRequestError::invalid_argument;
    wfrac_protocol::AirconFrameView frame{};
    if (wfrac_protocol::parse_aircon_stat(raw_aircon_stat, &frame) != wfrac_protocol::FrameError::none ||
        !frame.command.crc_valid || !frame.receive.crc_valid) {
      return ApiRequestError::invalid_aircon_stat;
    }
    base64_size = wfrac_protocol::base64_encoded_size(raw_aircon_stat.size);
    if (base64_size == 0U) return ApiRequestError::size_overflow;
  }

  TimeSample sample{};
  const ApiRequestError time_error = validate_time(time_provider, &sample);
  if (time_error != ApiRequestError::none) return time_error;

  std::size_t required_size = 0;
  if (!calculate_size(kind, context, aircon_id, sample.unix_seconds, base64_size, &required_size)) {
    return ApiRequestError::size_overflow;
  }
  if (required_size > output.size) return ApiRequestError::output_too_small;

  std::size_t cursor = 0;
  append_literal("{\"apiVer\":\"", output.data, &cursor);
  append_escaped(context.api_version, output.data, &cursor);
  append_literal("\",\"command\":\"", output.data, &cursor);
  append_literal(kind == RequestKind::get_device_info ? "getDeviceInfo"
                                                        : (kind == RequestKind::get_aircon_stat ? "getAirconStat" : "setAirconStat"),
                 output.data, &cursor);
  append_literal("\",\"deviceId\":\"", output.data, &cursor);
  append_escaped(context.device_id, output.data, &cursor);
  append_literal("\",\"operatorId\":\"", output.data, &cursor);
  append_escaped(context.operator_id, output.data, &cursor);
  append_literal("\",\"timestamp\":", output.data, &cursor);
  append_positive_decimal(sample.unix_seconds, output.data, &cursor);
  if (kind == RequestKind::get_device_info) {
    append_literal("}", output.data, &cursor);
  } else {
    append_literal(",\"contents\":{\"airconId\":\"", output.data, &cursor);
    append_escaped(aircon_id, output.data, &cursor);
    if (kind == RequestKind::get_aircon_stat) {
      append_literal("\"}}", output.data, &cursor);
    } else {
      append_literal("\",\"airconStat\":\"", output.data, &cursor);
      std::size_t encoded_size = 0;
      if (wfrac_protocol::base64_encode(raw_aircon_stat, {output.data + cursor, base64_size}, &encoded_size) !=
              wfrac_protocol::Base64Error::none ||
          encoded_size != base64_size) {
        reset_request(request);
        return ApiRequestError::base64_failure;
      }
      cursor += encoded_size;
      append_literal("\"}}", output.data, &cursor);
    }
  }
  request->path = kind == RequestKind::get_device_info
                      ? kGetDeviceInfoPath
                      : (kind == RequestKind::get_aircon_stat ? kGetAirconStatPath : kSetAirconStatPath);
  request->body = {output.data, cursor};
  request->timestamp = sample.unix_seconds;
  request->time_quality = sample.quality;
  return ApiRequestError::none;
}

}  // namespace

ApiRequestError build_get_device_info_request(const ApiClientContext& context, ITimeProvider* time_provider,
                                              wfrac_protocol::MutableByteView output, ApiRequestView* request) {
  return build_request(RequestKind::get_device_info, context, nullptr, {}, time_provider, output, request);
}

ApiRequestError build_get_aircon_stat_request(const ApiClientContext& context, const char* aircon_id,
                                              ITimeProvider* time_provider, wfrac_protocol::MutableByteView output,
                                              ApiRequestView* request) {
  return build_request(RequestKind::get_aircon_stat, context, aircon_id, {}, time_provider, output, request);
}

ApiRequestError build_set_aircon_stat_request(const ApiClientContext& context, const char* aircon_id,
                                              wfrac_protocol::ByteView raw_aircon_stat, ITimeProvider* time_provider,
                                              wfrac_protocol::MutableByteView output, ApiRequestView* request) {
  return build_request(RequestKind::set_aircon_stat, context, aircon_id, raw_aircon_stat, time_provider, output,
                       request);
}

}  // namespace wfrac_api
