#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "wfrac_transport/scheme_policy.hpp"
#include "wfrac_transport/transport.hpp"

namespace {

class FakeTransport final : public wfrac_transport::ITransport {
 public:
  void script(wfrac_transport::TransportError error, int http_status, const uint8_t* body, std::size_t body_size) {
    scripted_error_ = error;
    scripted_http_status_ = http_status;
    scripted_body_.clear();
    if (body_size != 0U) scripted_body_.assign(body, body + body_size);
  }

  wfrac_transport::TransportError perform(const wfrac_transport::TransportRequest& request,
                                          wfrac_transport::MutableByteView response_buffer,
                                          wfrac_transport::TransportResponse* response) override {
    if (response != nullptr) *response = {};
    if (response == nullptr || request.endpoint.host == nullptr || request.path == nullptr || request.timeout_ms == 0U ||
        (request.body.size != 0U && request.body.data == nullptr) ||
        (response_buffer.size != 0U && response_buffer.data == nullptr)) {
      return wfrac_transport::TransportError::invalid_argument;
    }

    ++perform_calls;
    endpoint = request.endpoint;
    captured_host = request.endpoint.host;
    captured_path = request.path;
    captured_timeout_ms = request.timeout_ms;
    captured_body.assign(request.body.data, request.body.data + request.body.size);

    if (scripted_error_ != wfrac_transport::TransportError::none) return scripted_error_;
    response->http_status = scripted_http_status_;
    if (scripted_body_.size() > response_buffer.size) return wfrac_transport::TransportError::response_too_large;
    if (!scripted_body_.empty()) std::memcpy(response_buffer.data, scripted_body_.data(), scripted_body_.size());
    response->body_size = scripted_body_.size();
    return wfrac_transport::TransportError::none;
  }

  std::size_t perform_calls = 0;
  wfrac_transport::TransportEndpoint endpoint{};
  std::string captured_host;
  std::string captured_path;
  uint32_t captured_timeout_ms = 0;
  std::vector<uint8_t> captured_body;

 private:
  wfrac_transport::TransportError scripted_error_ = wfrac_transport::TransportError::none;
  int scripted_http_status_ = 0;
  std::vector<uint8_t> scripted_body_;
};

wfrac_transport::TransportRequest make_request(wfrac_transport::TransportScheme scheme) {
  static constexpr char kHost[] = "example.local";
  static constexpr char kPath[] = "/test";
  return {{scheme, kHost, 51443U}, kPath, {}, 5000U};
}

void assert_no_response(const wfrac_transport::TransportResponse& response) {
  assert(response.http_status == 0);
  assert(response.body_size == 0U);
}

void assert_stop_unconfirmed(const wfrac_transport::SchemeAttemptDecision& decision) {
  assert(decision.error == wfrac_transport::SchemePolicyError::none);
  assert(decision.next_action == wfrac_transport::SchemeNextAction::stop);
  assert(!decision.scheme_confirmed);
}

}  // namespace

int main() {
  {
    FakeTransport transport;
    const uint8_t scripted_body[] = {'a', 'b', 'c'};
    const uint8_t request_body[] = {0x00U, 0xFFU, 0x80U};
    transport.script(wfrac_transport::TransportError::none, 200, scripted_body, sizeof(scripted_body));
    auto request = make_request(wfrac_transport::TransportScheme::https);
    request.body = {request_body, sizeof(request_body)};
    uint8_t response_body[8]{};
    wfrac_transport::TransportResponse response{};
    assert(transport.perform(request, {response_body, sizeof(response_body)}, &response) ==
           wfrac_transport::TransportError::none);
    assert(transport.perform_calls == 1U && transport.endpoint.scheme == wfrac_transport::TransportScheme::https);
    assert(transport.captured_host == "example.local" && transport.captured_path == "/test" &&
           transport.captured_timeout_ms == 5000U && transport.captured_body == std::vector<uint8_t>(request_body, request_body + 3));
    assert(response.http_status == 200 && response.body_size == 3U);
    assert(std::memcmp(response_body, scripted_body, sizeof(scripted_body)) == 0);
  }

  for (const int status : {400, 429}) {
    FakeTransport transport;
    static constexpr uint8_t kBody[] = {'{', '"', 'r', '"', ':', '2', '}'};
    transport.script(wfrac_transport::TransportError::none, status, kBody, sizeof(kBody));
    uint8_t response_body[16]{};
    wfrac_transport::TransportResponse response{};
    assert(transport.perform(make_request(wfrac_transport::TransportScheme::http), {response_body, sizeof(response_body)},
                             &response) == wfrac_transport::TransportError::none);
    assert(response.http_status == status && response.body_size == sizeof(kBody));
    wfrac_transport::SchemeCache cache{};
    const auto decision = wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::http, wfrac_transport::TransportError::none);
    assert(decision.scheme_confirmed && decision.next_action == wfrac_transport::SchemeNextAction::stop);
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::http_confirmed);
  }

  for (const auto error : {wfrac_transport::TransportError::timeout, wfrac_transport::TransportError::connection_failure,
                           wfrac_transport::TransportError::tls_failure}) {
    FakeTransport transport;
    transport.script(error, 200, nullptr, 0U);
    uint8_t response_body[4]{};
    wfrac_transport::TransportResponse response{99, 99U};
    assert(transport.perform(make_request(wfrac_transport::TransportScheme::https), {response_body, sizeof(response_body)},
                             &response) == error);
    assert(transport.perform_calls == 1U);
    assert_no_response(response);
  }

  {
    FakeTransport transport;
    static constexpr uint8_t kBody[] = {'a', 'b', 'c'};
    transport.script(wfrac_transport::TransportError::none, 200, kBody, sizeof(kBody));
    uint8_t response_body[2]{};
    wfrac_transport::TransportResponse response{99, 99U};
    assert(transport.perform(make_request(wfrac_transport::TransportScheme::https), {response_body, sizeof(response_body)},
                             &response) == wfrac_transport::TransportError::response_too_large);
    assert(transport.perform_calls == 1U && response.http_status == 200 && response.body_size == 0U);
  }

  {
    FakeTransport transport;
    uint8_t response_body[4]{};
    auto request = make_request(wfrac_transport::TransportScheme::http);
    wfrac_transport::TransportResponse response{};
    assert(transport.perform(request, {response_body, sizeof(response_body)}, nullptr) ==
           wfrac_transport::TransportError::invalid_argument);
    request.endpoint.host = nullptr;
    assert(transport.perform(request, {response_body, sizeof(response_body)}, &response) ==
           wfrac_transport::TransportError::invalid_argument);
    request = make_request(wfrac_transport::TransportScheme::http);
    request.path = nullptr;
    assert(transport.perform(request, {response_body, sizeof(response_body)}, &response) ==
           wfrac_transport::TransportError::invalid_argument);
    request = make_request(wfrac_transport::TransportScheme::http);
    request.timeout_ms = 0U;
    assert(transport.perform(request, {response_body, sizeof(response_body)}, &response) ==
           wfrac_transport::TransportError::invalid_argument);
    request = make_request(wfrac_transport::TransportScheme::http);
    request.body = {nullptr, 1U};
    assert(transport.perform(request, {response_body, sizeof(response_body)}, &response) ==
           wfrac_transport::TransportError::invalid_argument);
    request = make_request(wfrac_transport::TransportScheme::http);
    assert(transport.perform(request, {nullptr, 1U}, &response) == wfrac_transport::TransportError::invalid_argument);
    assert(transport.perform_calls == 0U);
  }

  {
    wfrac_transport::SchemeCache cache{};
    assert(wfrac_transport::select_scheme(cache) == wfrac_transport::TransportScheme::https);
    cache.knowledge = wfrac_transport::SchemeKnowledge::https_confirmed;
    assert(wfrac_transport::select_scheme(cache) == wfrac_transport::TransportScheme::https);
    cache.knowledge = wfrac_transport::SchemeKnowledge::http_confirmed;
    assert(wfrac_transport::select_scheme(cache) == wfrac_transport::TransportScheme::http);
  }

  {
    wfrac_transport::SchemeCache cache{};
    const auto decision = wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::none);
    assert(decision.error == wfrac_transport::SchemePolicyError::none && decision.scheme_confirmed);
    assert(decision.next_action == wfrac_transport::SchemeNextAction::stop);
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::https_confirmed);
    assert(wfrac_transport::select_scheme(cache) == wfrac_transport::TransportScheme::https);
  }

  for (const auto scheme : {wfrac_transport::TransportScheme::https, wfrac_transport::TransportScheme::http}) {
    wfrac_transport::SchemeCache cache{};
    const auto decision = wfrac_transport::observe_scheme_attempt(
        &cache, scheme, wfrac_transport::TransportError::response_too_large);
    assert(decision.error == wfrac_transport::SchemePolicyError::none && decision.scheme_confirmed);
    assert(decision.next_action == wfrac_transport::SchemeNextAction::stop);
    assert(cache.knowledge == (scheme == wfrac_transport::TransportScheme::https
                                   ? wfrac_transport::SchemeKnowledge::https_confirmed
                                   : wfrac_transport::SchemeKnowledge::http_confirmed));
  }

  {
    wfrac_transport::SchemeCache cache{};
    const auto https_decision = wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::scheme_mismatch);
    assert(https_decision.error == wfrac_transport::SchemePolicyError::none && !https_decision.scheme_confirmed);
    assert(https_decision.next_action == wfrac_transport::SchemeNextAction::retry_http_compatibility);
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::unknown);

    const auto http_decision = wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::http, wfrac_transport::TransportError::none);
    assert(http_decision.scheme_confirmed && http_decision.next_action == wfrac_transport::SchemeNextAction::stop);
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::http_confirmed);
    assert(wfrac_transport::select_scheme(cache) == wfrac_transport::TransportScheme::http);
  }

  for (const auto fallback_error : {wfrac_transport::TransportError::timeout,
                                    wfrac_transport::TransportError::connection_failure,
                                    wfrac_transport::TransportError::io_failure,
                                    wfrac_transport::TransportError::scheme_mismatch,
                                    wfrac_transport::TransportError::tls_failure}) {
    wfrac_transport::SchemeCache cache{};
    const auto https_decision = wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::scheme_mismatch);
    assert(https_decision.next_action == wfrac_transport::SchemeNextAction::retry_http_compatibility);
    const auto http_decision =
        wfrac_transport::observe_scheme_attempt(&cache, wfrac_transport::TransportScheme::http, fallback_error);
    assert_stop_unconfirmed(http_decision);
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::unknown);
  }

  {
    wfrac_transport::SchemeCache cache{};
    assert_stop_unconfirmed(wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::tls_identity_failure));
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::unknown);
    assert_stop_unconfirmed(wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::tls_failure));
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::unknown);
  }

  for (const auto error : {wfrac_transport::TransportError::timeout,
                           wfrac_transport::TransportError::connection_failure,
                           wfrac_transport::TransportError::io_failure}) {
    for (const auto knowledge : {wfrac_transport::SchemeKnowledge::unknown,
                                 wfrac_transport::SchemeKnowledge::https_confirmed,
                                 wfrac_transport::SchemeKnowledge::http_confirmed}) {
      wfrac_transport::SchemeCache cache{knowledge};
      const auto attempted_scheme = knowledge == wfrac_transport::SchemeKnowledge::http_confirmed
                                        ? wfrac_transport::TransportScheme::http
                                        : wfrac_transport::TransportScheme::https;
      assert_stop_unconfirmed(wfrac_transport::observe_scheme_attempt(&cache, attempted_scheme, error));
      assert(cache.knowledge == knowledge);
    }
  }

  for (const auto error : {wfrac_transport::TransportError::timeout,
                           wfrac_transport::TransportError::connection_failure,
                           wfrac_transport::TransportError::tls_failure,
                           wfrac_transport::TransportError::tls_identity_failure,
                           wfrac_transport::TransportError::scheme_mismatch,
                           wfrac_transport::TransportError::io_failure}) {
    wfrac_transport::SchemeCache https_cache{wfrac_transport::SchemeKnowledge::https_confirmed};
    assert_stop_unconfirmed(wfrac_transport::observe_scheme_attempt(
        &https_cache, wfrac_transport::TransportScheme::https, error));
    assert(https_cache.knowledge == wfrac_transport::SchemeKnowledge::https_confirmed);

    wfrac_transport::SchemeCache http_cache{wfrac_transport::SchemeKnowledge::http_confirmed};
    assert_stop_unconfirmed(
        wfrac_transport::observe_scheme_attempt(&http_cache, wfrac_transport::TransportScheme::http, error));
    assert(http_cache.knowledge == wfrac_transport::SchemeKnowledge::http_confirmed);
  }

  {
    const auto decision = wfrac_transport::observe_scheme_attempt(
        nullptr, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::none);
    assert(decision.error == wfrac_transport::SchemePolicyError::invalid_argument);
    assert(decision.next_action == wfrac_transport::SchemeNextAction::stop && !decision.scheme_confirmed);

    wfrac_transport::SchemeCache cache{wfrac_transport::SchemeKnowledge::https_confirmed};
    assert_stop_unconfirmed(wfrac_transport::observe_scheme_attempt(
        &cache, wfrac_transport::TransportScheme::https, wfrac_transport::TransportError::invalid_argument));
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::https_confirmed);
    assert(wfrac_transport::clear_scheme_cache(nullptr) == wfrac_transport::SchemePolicyError::invalid_argument);
  }

  for (const auto knowledge : {wfrac_transport::SchemeKnowledge::https_confirmed,
                               wfrac_transport::SchemeKnowledge::http_confirmed}) {
    wfrac_transport::SchemeCache cache{knowledge};
    assert(wfrac_transport::clear_scheme_cache(&cache) == wfrac_transport::SchemePolicyError::none);
    assert(cache.knowledge == wfrac_transport::SchemeKnowledge::unknown);
    assert(wfrac_transport::select_scheme(cache) == wfrac_transport::TransportScheme::https);
  }

  return 0;
}
