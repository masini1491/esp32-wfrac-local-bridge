#include "diagnostic_form.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <limits>

namespace diagnostic {
void erase(void* data, std::size_t size) {
  auto* bytes = static_cast<volatile unsigned char*>(data);
  if (bytes != nullptr) for (std::size_t i = 0; i < size; ++i) bytes[i] = 0;
}

bool positive_decimal(const char* text, int64_t* value) {
  if (text == nullptr || *text == '\0' || value == nullptr) return false;
  int64_t n = 0;
  for (; *text != '\0'; ++text) {
    if (*text < '0' || *text > '9') return false;
    const int digit = *text - '0';
    if (n > (std::numeric_limits<int64_t>::max() - digit) / 10) return false;
    n = n * 10 + digit;
  }
  if (n == 0) return false;
  *value = n;
  return true;
}

namespace {
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
InputError decode(const char* data, std::size_t size, char* out, std::size_t capacity) {
  std::size_t written = 0;
  for (std::size_t i = 0; i < size; ++i) {
    unsigned char c = static_cast<unsigned char>(data[i]);
    if (c == '%') {
      if (size - i < 3 || hex(data[i + 1]) < 0 || hex(data[i + 2]) < 0) return InputError::malformed_form;
      c = static_cast<unsigned char>(hex(data[i + 1]) * 16 + hex(data[i + 2]));
      i += 2;
    } else if (c == '+') c = ' ';
    if (c == 0 || c == 127 || (c < 32 && c != '\n' && c != '\r')) return InputError::invalid_field;
    if (written + 1 >= capacity) return InputError::too_large;
    out[written++] = static_cast<char>(c);
  }
  out[written] = '\0';
  return InputError::none;
}
bool ipv4(const char* host) {
  // Numeric unicast endpoint only: no DNS, URL, userinfo, port or path injection.
  unsigned first = 0;
  for (unsigned part = 0; part < 4; ++part) {
    unsigned n = 0, digits = 0;
    while (*host >= '0' && *host <= '9') {
      n = n * 10 + static_cast<unsigned>(*host++ - '0');
      if (++digits > 3 || n > 255) return false;
    }
    if (digits == 0) return false;
    if (part == 0) first = n;
    if (part == 3) { if (*host != '\0') return false; }
    else if (*host++ != '.') return false;
  }
  return first != 0 && first != 127 && first < 224;
}
bool single_line(const char* text) { return std::strchr(text, '\r') == nullptr && std::strchr(text, '\n') == nullptr; }
}  // namespace

InputError validate_input(const Input& in) {
  if (in.ssid.back() != '\0' || in.password.back() != '\0' || in.host.back() != '\0' ||
      in.device_id.back() != '\0' || in.operator_id.back() != '\0' || in.certificate.back() != '\0' ||
      in.common_name.back() != '\0' || in.nonce.back() != '\0') return InputError::invalid_argument;
  if (in.ssid[0] == '\0' || in.host[0] == '\0' || in.device_id[0] == '\0' || in.operator_id[0] == '\0' ||
      in.time_seed <= 0 || in.port == 0) return InputError::missing_field;
  if (!ipv4(in.host.data()) || !single_line(in.ssid.data()) || !single_line(in.password.data()) ||
      !single_line(in.device_id.data()) || !single_line(in.operator_id.data()) || !single_line(in.common_name.data()))
    return InputError::invalid_field;
  if (in.scheme != Scheme::http && in.scheme != Scheme::https) return InputError::invalid_field;
  if (in.scheme == Scheme::https) {
    constexpr char begin[] = "-----BEGIN CERTIFICATE-----";
    if (std::strncmp(in.certificate.data(), begin, sizeof(begin) - 1) != 0 ||
        std::strstr(in.certificate.data(), "-----END CERTIFICATE-----") == nullptr) return InputError::trust_required;
    // This is a PEM presence guard, not a claim of cryptographic validity.
  }
  return InputError::none;
}

InputError parse_form(const char* data, std::size_t size, Input* output) {
  if (output == nullptr) return InputError::invalid_argument;
  *output = {};
  auto fail = [output](InputError error) { erase(output, sizeof(*output)); return error; };
  if (data == nullptr || size == 0) return fail(InputError::invalid_argument);
  if (size > kFormLimit) return fail(InputError::too_large);
  char port[24]{}, seed[24]{}, scheme[8]{};
  struct Field { const char* name; char* data; std::size_t capacity; };
  Field fields[] = {
    {"ssid", output->ssid.data(), output->ssid.size()}, {"password", output->password.data(), output->password.size()},
    {"host", output->host.data(), output->host.size()}, {"device_id", output->device_id.data(), output->device_id.size()},
    {"operator_id", output->operator_id.data(), output->operator_id.size()}, {"certificate", output->certificate.data(), output->certificate.size()},
    {"common_name", output->common_name.data(), output->common_name.size()}, {"nonce", output->nonce.data(), output->nonce.size()},
    {"port", port, sizeof(port)}, {"time_seed", seed, sizeof(seed)}, {"scheme", scheme, sizeof(scheme)}
  };
  unsigned seen = 0;
  std::size_t start = 0;
  while (start < size) {
    std::size_t end = start;
    while (end < size && data[end] != '&') ++end;
    std::size_t equal = start;
    while (equal < end && data[equal] != '=') ++equal;
    if (equal == end || equal == start) return fail(InputError::malformed_form);
    char name[24]{};
    auto error = decode(data + start, equal - start, name, sizeof(name));
    if (error != InputError::none) return fail(error);
    unsigned index = 0;
    while (index < sizeof(fields) / sizeof(fields[0]) && std::strcmp(name, fields[index].name) != 0) ++index;
    if (index == sizeof(fields) / sizeof(fields[0])) return fail(InputError::invalid_field);
    const unsigned bit = 1U << index;
    if ((seen & bit) != 0) return fail(InputError::duplicate_field);
    seen |= bit;
    error = decode(data + equal + 1, end - equal - 1, fields[index].data, fields[index].capacity);
    if (error != InputError::none) return fail(error);
    if (end + 1 == size) return fail(InputError::malformed_form);
    start = end + 1;
  }
  constexpr unsigned required = (1U << 0) | (1U << 2) | (1U << 3) | (1U << 4) | (1U << 7) | (1U << 8) | (1U << 9) | (1U << 10);
  if ((seen & required) != required || output->nonce[0] == '\0') return fail(InputError::missing_field);
  int64_t parsed_port = 0;
  if (!positive_decimal(port, &parsed_port) || parsed_port > 65535 || !positive_decimal(seed, &output->time_seed))
    return fail(InputError::invalid_field);
  output->port = static_cast<uint16_t>(parsed_port);
  if (std::strcmp(scheme, "http") == 0) output->scheme = Scheme::http;
  else if (std::strcmp(scheme, "https") == 0) output->scheme = Scheme::https;
  else return fail(InputError::invalid_field);
  const auto error = validate_input(*output);
  if (error != InputError::none) return fail(error);
  return InputError::none;
}

bool format_result(const Result& result, char* output, std::size_t capacity) {
  if (output == nullptr || capacity == 0) return false;
  output[0] = '\0';
  constexpr const char* stages[] = {"CONFIG", "TIME", "WIFI_INIT", "WIFI_CONNECT", "GOT_IP", "API_REQUEST", "TRANSPORT", "API_ENVELOPE", "DEVICE_INFO", "DONE"};
  const auto index = static_cast<unsigned>(result.stage);
  const char* stage = index < sizeof(stages) / sizeof(stages[0]) ? stages[index] : "UNKNOWN";
  const int n = std::snprintf(output, capacity,
      "<!doctype html><meta charset=utf-8><title>Diagnostic result</title><pre>Stage=%s Code=%d HTTP=%d\n"
      "Envelope=%d RawResult=%" PRId64 " DeviceInfo=%d\nSTA-attempts=%u Request-attempts=%u Cleanup=%d</pre>"
      "<p>Read-only diagnostic, not hardware or physical-actuation confirmation.</p><a href='/'>New explicit probe</a>",
      stage, result.code, result.http_status, result.has_result, result.api_result, result.device_info_parsed,
      result.connect_attempts, result.request_attempts, result.cleanup_ok);
  if (n < 0 || static_cast<std::size_t>(n) >= capacity) { output[0] = '\0'; return false; }
  return true;
}
}  // namespace diagnostic
