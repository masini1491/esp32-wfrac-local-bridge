#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

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

  return 0;
}
