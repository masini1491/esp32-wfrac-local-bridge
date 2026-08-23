#include "wfrac_client/request_executor.hpp"

namespace wfrac_client {
namespace {

bool is_nonempty(const char* value) { return value != nullptr && value[0] != '\0'; }

bool is_valid(const wfrac_api::ApiRequestView& api_request, const ClientEndpoint& endpoint,
              wfrac_transport::SchemeCache* scheme_cache, wfrac_transport::ITransport* transport,
              wfrac_transport::MutableByteView response_buffer, wfrac_transport::TransportResponse* response) {
  return is_nonempty(api_request.path) && (api_request.body.size == 0 || api_request.body.data != nullptr) &&
         is_nonempty(endpoint.host) && endpoint.port != 0 && endpoint.timeout_ms != 0 && scheme_cache != nullptr &&
         transport != nullptr && response != nullptr && (response_buffer.size == 0 || response_buffer.data != nullptr);
}

wfrac_transport::TransportRequest make_request(const wfrac_api::ApiRequestView& api_request,
                                                const ClientEndpoint& endpoint,
                                                wfrac_transport::TransportScheme scheme) {
  return {{scheme, endpoint.host, endpoint.port}, api_request.path,
          {api_request.body.data, api_request.body.size}, endpoint.timeout_ms};
}

}  // namespace

wfrac_transport::TransportError execute_api_request(
    const wfrac_api::ApiRequestView& api_request, const ClientEndpoint& endpoint,
    wfrac_transport::SchemeCache* scheme_cache, wfrac_transport::ITransport* transport,
    wfrac_transport::MutableByteView response_buffer, wfrac_transport::TransportResponse* response,
    RequestExecutionReport* report) {
  if (response != nullptr) *response = {};
  if (report != nullptr) *report = {};
  if (!is_valid(api_request, endpoint, scheme_cache, transport, response_buffer, response)) {
    return wfrac_transport::TransportError::invalid_argument;
  }

  const wfrac_transport::TransportScheme first_scheme = wfrac_transport::select_scheme(*scheme_cache);
  if (report != nullptr) {
    report->first_scheme = first_scheme;
    report->final_scheme = first_scheme;
    report->attempts = 1;
  }
  const auto first_request = make_request(api_request, endpoint, first_scheme);
  const auto first_error = transport->perform(first_request, response_buffer, response);
  const auto decision = wfrac_transport::observe_scheme_attempt(scheme_cache, first_scheme, first_error);
  if (decision.error != wfrac_transport::SchemePolicyError::none ||
      decision.next_action != wfrac_transport::SchemeNextAction::retry_http_compatibility) {
    return decision.error == wfrac_transport::SchemePolicyError::none ? first_error
                                                                       : wfrac_transport::TransportError::invalid_argument;
  }

  *response = {};
  const auto fallback_request = make_request(api_request, endpoint, wfrac_transport::TransportScheme::http);
  if (report != nullptr) {
    report->attempts = 2;
    report->final_scheme = wfrac_transport::TransportScheme::http;
    report->http_compatibility_fallback_used = true;
  }
  const auto second_error = transport->perform(fallback_request, response_buffer, response);
  (void)wfrac_transport::observe_scheme_attempt(scheme_cache, wfrac_transport::TransportScheme::http, second_error);
  return second_error;
}

}  // namespace wfrac_client
