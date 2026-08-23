#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_api/api_request.hpp"
#include "wfrac_transport/scheme_policy.hpp"
#include "wfrac_transport/transport.hpp"

namespace wfrac_client {

struct ClientEndpoint {
  const char* host = nullptr;
  uint16_t port = 51443;
  uint32_t timeout_ms = 0;
};

struct RequestExecutionReport {
  std::size_t attempts = 0;
  wfrac_transport::TransportScheme first_scheme = wfrac_transport::TransportScheme::https;
  wfrac_transport::TransportScheme final_scheme = wfrac_transport::TransportScheme::https;
  bool http_compatibility_fallback_used = false;
};

wfrac_transport::TransportError execute_api_request(
    const wfrac_api::ApiRequestView& api_request, const ClientEndpoint& endpoint,
    wfrac_transport::SchemeCache* scheme_cache, wfrac_transport::ITransport* transport,
    wfrac_transport::MutableByteView response_buffer, wfrac_transport::TransportResponse* response,
    RequestExecutionReport* report = nullptr);

}  // namespace wfrac_client
