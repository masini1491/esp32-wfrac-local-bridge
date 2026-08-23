#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "wfrac_api/api_request.hpp"
#include "wfrac_client/request_executor.hpp"

namespace {

struct Step { wfrac_transport::TransportError error; int status; const char* body; };

class ScriptedTransport final : public wfrac_transport::ITransport {
 public:
  explicit ScriptedTransport(std::vector<Step> steps) : steps_(steps) {}
  wfrac_transport::TransportError perform(const wfrac_transport::TransportRequest& request,
                                           wfrac_transport::MutableByteView output,
                                           wfrac_transport::TransportResponse* response) override {
    assert(calls_.size() < steps_.size());
    calls_.push_back(request);
    const Step& step = steps_[calls_.size() - 1U];
    if (step.error == wfrac_transport::TransportError::none && step.body != nullptr) {
      const std::size_t length = std::strlen(step.body);
      assert(length <= output.size);
      std::memcpy(output.data, step.body, length);
      response->http_status = step.status;
      response->body_size = length;
    }
    return step.error;
  }
  const std::vector<wfrac_transport::TransportRequest>& calls() const { return calls_; }
 private:
  std::vector<Step> steps_;
  std::vector<wfrac_transport::TransportRequest> calls_;
};

class FakeTimeProvider final : public wfrac_api::ITimeProvider {
 public:
  wfrac_api::TimeSample now() override { ++calls; return {1700000000, wfrac_api::TimeQuality::trusted}; }
  int calls = 0;
};

wfrac_api::ApiRequestView request(const uint8_t* body, std::size_t size) {
  return {"/beaver/command/test", {body, size}, 1, wfrac_api::TimeQuality::trusted};
}

}  // namespace

int main() {
  using wfrac_transport::SchemeKnowledge;
  using wfrac_transport::TransportError;
  constexpr uint8_t body[] = {0x00, 0x80, 0xFF};
  uint8_t response_bytes[32]{};
  const wfrac_client::ClientEndpoint endpoint{"192.0.2.1", 51443, 1000};

  { // Invalid inputs never call transport and reset the report.
    ScriptedTransport transport({}); wfrac_transport::SchemeCache cache{}; wfrac_transport::TransportResponse response{7, 7};
    wfrac_client::RequestExecutionReport report{9, wfrac_transport::TransportScheme::http,
                                                 wfrac_transport::TransportScheme::http, true};
    auto invalid = request(nullptr, 1);
    assert(wfrac_client::execute_api_request(invalid, endpoint, &cache, &transport, {response_bytes, sizeof(response_bytes)},
                                             &response, &report) == TransportError::invalid_argument);
    assert(transport.calls().empty() && cache.knowledge == SchemeKnowledge::unknown && response.body_size == 0 && report.attempts == 0);
  }
  { // HTTPS success and application non-2xx confirm HTTPS without fallback.
    ScriptedTransport transport({{TransportError::none, 429, "x"}}); wfrac_transport::SchemeCache cache{};
    wfrac_transport::TransportResponse response{}; wfrac_client::RequestExecutionReport report{};
    assert(wfrac_client::execute_api_request(request(body, sizeof(body)), endpoint, &cache, &transport,
                                             {response_bytes, sizeof(response_bytes)}, &response, &report) == TransportError::none);
    assert(report.attempts == 1 && !report.http_compatibility_fallback_used && response.http_status == 429 &&
           cache.knowledge == SchemeKnowledge::https_confirmed && transport.calls()[0].endpoint.scheme == wfrac_transport::TransportScheme::https);
  }
  { // Only an unknown HTTPS scheme mismatch receives one HTTP fallback with identical request fields/body.
    ScriptedTransport transport({{TransportError::scheme_mismatch, 0, nullptr}, {TransportError::none, 200, "ok"}});
    wfrac_transport::SchemeCache cache{}; wfrac_transport::TransportResponse response{}; wfrac_client::RequestExecutionReport report{};
    assert(wfrac_client::execute_api_request(request(body, sizeof(body)), endpoint, &cache, &transport,
                                             {response_bytes, sizeof(response_bytes)}, &response, &report) == TransportError::none);
    assert(report.attempts == 2 && report.http_compatibility_fallback_used && report.first_scheme == wfrac_transport::TransportScheme::https &&
           report.final_scheme == wfrac_transport::TransportScheme::http && cache.knowledge == SchemeKnowledge::http_confirmed && response.http_status == 200);
    const auto& first = transport.calls()[0]; const auto& second = transport.calls()[1];
    assert(first.endpoint.scheme == wfrac_transport::TransportScheme::https && second.endpoint.scheme == wfrac_transport::TransportScheme::http &&
           first.endpoint.host == second.endpoint.host && first.endpoint.port == second.endpoint.port && first.path == second.path &&
           first.timeout_ms == second.timeout_ms && first.body.size == second.body.size && std::memcmp(first.body.data, second.body.data, first.body.size) == 0);
  }
  { // Fallback stops after its final error; response-too-large confirms HTTP.
    ScriptedTransport transport({{TransportError::scheme_mismatch, 0, nullptr}, {TransportError::response_too_large, 0, nullptr}});
    wfrac_transport::SchemeCache cache{}; wfrac_transport::TransportResponse response{};
    assert(wfrac_client::execute_api_request(request(body, sizeof(body)), endpoint, &cache, &transport,
                                             {response_bytes, sizeof(response_bytes)}, &response) == TransportError::response_too_large);
    assert(transport.calls().size() == 2 && cache.knowledge == SchemeKnowledge::http_confirmed);
  }
  for (const auto error : {TransportError::tls_identity_failure, TransportError::tls_failure, TransportError::timeout}) {
    ScriptedTransport transport({{error, 0, nullptr}}); wfrac_transport::SchemeCache cache{}; wfrac_transport::TransportResponse response{};
    assert(wfrac_client::execute_api_request(request(body, sizeof(body)), endpoint, &cache, &transport,
                                             {response_bytes, sizeof(response_bytes)}, &response) == error);
    assert(transport.calls().size() == 1 && cache.knowledge == SchemeKnowledge::unknown);
  }
  { // Confirmed schemes remain sticky and never create an automatic opposite-scheme attempt.
    ScriptedTransport https_transport({{TransportError::scheme_mismatch, 0, nullptr}});
    wfrac_transport::SchemeCache https_cache{SchemeKnowledge::https_confirmed}; wfrac_transport::TransportResponse response{};
    (void)wfrac_client::execute_api_request(request(body, sizeof(body)), endpoint, &https_cache, &https_transport,
                                            {response_bytes, sizeof(response_bytes)}, &response);
    assert(https_transport.calls().size() == 1 && https_cache.knowledge == SchemeKnowledge::https_confirmed);
    ScriptedTransport http_transport({{TransportError::connection_failure, 0, nullptr}});
    wfrac_transport::SchemeCache http_cache{SchemeKnowledge::http_confirmed};
    (void)wfrac_client::execute_api_request(request(body, sizeof(body)), endpoint, &http_cache, &http_transport,
                                            {response_bytes, sizeof(response_bytes)}, &response);
    assert(http_transport.calls().size() == 1 && http_transport.calls()[0].endpoint.scheme == wfrac_transport::TransportScheme::http &&
           http_cache.knowledge == SchemeKnowledge::http_confirmed);
  }
  { // Building once calls TimeProvider once even when executor falls back.
    FakeTimeProvider time; wfrac_api::ApiClientContext context{"1.0", "device", "operator"}; uint8_t api_body[256]{}; wfrac_api::ApiRequestView api{};
    assert(wfrac_api::build_get_device_info_request(context, &time, {api_body, sizeof(api_body)}, &api) == wfrac_api::ApiRequestError::none);
    ScriptedTransport transport({{TransportError::scheme_mismatch, 0, nullptr}, {TransportError::none, 200, "{}"}});
    wfrac_transport::SchemeCache cache{}; wfrac_transport::TransportResponse response{};
    assert(wfrac_client::execute_api_request(api, endpoint, &cache, &transport, {response_bytes, sizeof(response_bytes)}, &response) == TransportError::none);
    assert(time.calls == 1 && transport.calls().size() == 2);
  }
  return 0;
}
