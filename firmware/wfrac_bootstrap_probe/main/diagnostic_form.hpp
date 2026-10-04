#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace diagnostic {

constexpr std::size_t kFormLimit = 16384U;
enum class Scheme { http, https };
enum class InputError { none, invalid_argument, malformed_form, too_large, duplicate_field, invalid_field, missing_field, trust_required };

// Firmware-local RAM storage, never a portable component or persistence API.
struct Input {
  std::array<char, 33> ssid{};
  std::array<char, 65> password{};
  std::array<char, 129> host{};
  std::array<char, 129> device_id{};
  std::array<char, 129> operator_id{};
  std::array<char, 4097> certificate{};
  std::array<char, 129> common_name{};
  std::array<char, 33> nonce{};
  int64_t time_seed = 0;
  uint16_t port = 0;
  Scheme scheme = Scheme::https;
};

enum class Stage { config, time, wifi_init, wifi_connect, got_ip, request, transport, envelope, device_info, done };
struct Result {
  Stage stage = Stage::config;
  int code = 0;
  int http_status = 0;
  bool has_result = false;
  int64_t api_result = 0;
  bool device_info_parsed = false;
  bool cleanup_ok = true;
  unsigned connect_attempts = 0;
  unsigned request_attempts = 0;
};

void erase(void* data, std::size_t size);
bool positive_decimal(const char* text, int64_t* value);
InputError validate_input(const Input& input);
// Strict application/x-www-form-urlencoded, explicit size, no heap/truncation.
// Failure erases output. Required fields and nonce must occur exactly once.
InputError parse_form(const char* data, std::size_t size, Input* output);
// Only numeric diagnostic metadata and fixed labels; no input/response strings.
bool format_result(const Result& result, char* output, std::size_t capacity);

struct EraseOnExit {
  void* data;
  std::size_t size;
  ~EraseOnExit() { erase(data, size); }
};

}  // namespace diagnostic
