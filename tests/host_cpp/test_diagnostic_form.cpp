#include "diagnostic_form.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

namespace {
const std::string base = "ssid=synthetic-lab&password=synthetic-password&host=192.0.2.10&device_id=synthetic-device&operator_id=synthetic-operator&nonce=synthetic-nonce&port=51443&time_seed=5000000000&scheme=http";
diagnostic::Input input{};
diagnostic::InputError parse(const std::string& text) { return diagnostic::parse_form(text.data(), text.size(), &input); }
std::string change(std::string text, const std::string& from, const std::string& to) {
  const auto pos = text.find(from); assert(pos != std::string::npos); text.replace(pos, from.size(), to); return text;
}
void rejected(const std::string& text, diagnostic::InputError expected) {
  input.port = 123; input.operator_id[0] = 'x';
  assert(parse(text) == expected);
  assert(input.port == 0 && input.time_seed == 0 && input.operator_id[0] == '\0' && input.password[0] == '\0');
}
}  // namespace
int main() {
  using diagnostic::InputError;
  assert(parse(base) == InputError::none);
  assert(input.time_seed == 5000000000LL && input.port == 51443 && input.scheme == diagnostic::Scheme::http);
  assert(std::strcmp(input.password.data(), "synthetic-password") == 0);
  assert(parse(change(base, "synthetic-lab", std::string(32, 's'))) == InputError::none);
  assert(parse(change(base, "synthetic-password", std::string(64, 'a'))) == InputError::none);
  rejected(change(base, "synthetic-lab", std::string(33, 's')), InputError::too_large);
  rejected(change(base, "synthetic-password", std::string(65, 'a')), InputError::too_large);
  rejected(base + "&port=1", InputError::duplicate_field);
  rejected(base + "&unexpected=1", InputError::invalid_field);
  rejected(base + "&", InputError::malformed_form);
  rejected(change(base, "synthetic-lab", "%"), InputError::malformed_form);
  rejected(change(base, "synthetic-lab", "%GG"), InputError::malformed_form);
  rejected(change(base, "synthetic-lab", "%00"), InputError::invalid_field);
  rejected(change(base, "synthetic-lab", "%0d%0a"), InputError::invalid_field);
  rejected(change(base, "ssid=synthetic-lab&", ""), InputError::missing_field);
  rejected(change(base, "operator_id=synthetic-operator", "operator_id="), InputError::missing_field);
  rejected(change(base, "scheme=http", ""), InputError::malformed_form);
  rejected(change(base, "scheme=http", "scheme=auto"), InputError::invalid_field);
  rejected(change(base, "51443", "0"), InputError::invalid_field);
  rejected(change(base, "51443", "65536"), InputError::invalid_field);
  assert(parse(change(base, "51443", "65535")) == InputError::none);
  for (const auto* seed : {"0", "-1", "+1", "1e9", "9223372036854775808"})
    rejected(change(base, "5000000000", seed), InputError::invalid_field);
  assert(parse(change(base, "5000000000", "9223372036854775807")) == InputError::none);
  for (const auto* host : {"localhost", "http%3A%2F%2F192.0.2.1", "127.0.0.1", "224.0.0.1", "192.0.2.999"})
    rejected(change(base, "192.0.2.10", host), InputError::invalid_field);
  rejected(change(base, "scheme=http", "scheme=https"), InputError::trust_required);
  const auto https = change(base, "scheme=http", "scheme=https") +
      "&certificate=-----BEGIN+CERTIFICATE-----%0ATEST%0A-----END+CERTIFICATE-----&common_name=synthetic.example";
  assert(parse(https) == InputError::none && input.scheme == diagnostic::Scheme::https);
  assert(std::strchr(input.certificate.data(), '\n') != nullptr);
  rejected(base + "&nonce=again", InputError::duplicate_field);
  rejected(std::string(diagnostic::kFormLimit + 1, 'x'), InputError::too_large);
  assert(diagnostic::parse_form(nullptr, 1, &input) == InputError::invalid_argument);
  assert(diagnostic::parse_form(base.data(), base.size(), nullptr) == InputError::invalid_argument);
  assert(parse(base) == InputError::none);
  input.host.fill('x');
  assert(diagnostic::validate_input(input) == InputError::invalid_argument);
  diagnostic::Result result{};
  result.stage = diagnostic::Stage::done; result.has_result = true; result.api_result = 2;
  result.http_status = 429; result.connect_attempts = 1; result.request_attempts = 1;
  char html[640]{};
  assert(diagnostic::format_result(result, html, sizeof(html)));
  assert(std::strstr(html, "RawResult=2") != nullptr && std::strstr(html, "HTTP=429") != nullptr);
  for (const auto* secret : {"synthetic-password", "synthetic-operator", "synthetic-device", "192.0.2.10", "TEST"})
    assert(std::strstr(html, secret) == nullptr);
  const auto required = std::strlen(html) + 1;
  assert(diagnostic::format_result(result, html, required));
  assert(!diagnostic::format_result(result, html, required - 1) && html[0] == '\0');
  assert(!diagnostic::format_result(result, nullptr, 0));
  char temporary[] = "synthetic-secret";
  { diagnostic::EraseOnExit wipe{temporary, sizeof(temporary)}; }
  for (char c : temporary) assert(c == '\0');
  std::cout << "diagnostic form tests PASS (offline; no ESP-IDF/network execution)\n";
}
