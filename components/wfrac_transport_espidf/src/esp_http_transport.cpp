#include "wfrac_transport_espidf/esp_http_transport.hpp"

#include <cstring>
#include <limits>

#include "esp_http_client.h"

#include "wfrac_transport_espidf/error_classifier.hpp"

namespace wfrac_transport_espidf {
namespace {

struct ResponseCollector {
  wfrac_transport::MutableByteView output{};
  std::size_t written = 0;
  bool overflow = false;
};

esp_err_t collect_response_data(esp_http_client_event_t* event) {
  if (event == nullptr || event->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;

  auto* collector = static_cast<ResponseCollector*>(event->user_data);
  if (collector == nullptr || event->data_len < 0) return ESP_FAIL;
  if (event->data_len == 0 || collector->overflow) return ESP_OK;
  if (event->data == nullptr || collector->written > collector->output.size) {
    collector->overflow = true;
    return ESP_OK;
  }

  const std::size_t chunk_size = static_cast<std::size_t>(event->data_len);
  if (chunk_size > collector->output.size - collector->written) {
    collector->overflow = true;
    return ESP_OK;
  }

  std::memcpy(collector->output.data + collector->written, event->data, chunk_size);
  collector->written += chunk_size;
  return ESP_OK;
}

bool is_nonempty(const char* value) { return value != nullptr && value[0] != '\0'; }

bool is_valid_request(const wfrac_transport::TransportRequest& request,
                      wfrac_transport::MutableByteView response_buffer,
                      const EspHttpTransportConfig& config) {
  if (!is_nonempty(request.endpoint.host) || request.endpoint.port == 0 || !is_nonempty(request.path) ||
      request.timeout_ms == 0 || request.timeout_ms > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
      (request.body.size != 0 && request.body.data == nullptr) ||
      request.body.size > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
      (response_buffer.size != 0 && response_buffer.data == nullptr)) {
    return false;
  }

  return request.endpoint.scheme != wfrac_transport::TransportScheme::https ||
         config.tls.has_valid_pem_certificate();
}

}  // namespace

EspHttpTransport::EspHttpTransport(EspHttpTransportConfig config) : config_(config) {}

wfrac_transport::TransportError EspHttpTransport::perform(
    const wfrac_transport::TransportRequest& request, wfrac_transport::MutableByteView response_buffer,
    wfrac_transport::TransportResponse* response) {
  using wfrac_transport::TransportError;

  if (response != nullptr) *response = {};
  if (response == nullptr || !is_valid_request(request, response_buffer, config_)) {
    return TransportError::invalid_argument;
  }

  ResponseCollector collector{response_buffer};
  esp_http_client_config_t client_config{};
  client_config.host = request.endpoint.host;
  client_config.port = static_cast<int>(request.endpoint.port);
  client_config.path = request.path;
  client_config.method = HTTP_METHOD_POST;
  client_config.timeout_ms = static_cast<int>(request.timeout_ms);
  client_config.transport_type = request.endpoint.scheme == wfrac_transport::TransportScheme::https
                                     ? HTTP_TRANSPORT_OVER_SSL
                                     : HTTP_TRANSPORT_OVER_TCP;
  client_config.disable_auto_redirect = true;
  client_config.max_authorization_retries = -1;
  client_config.event_handler = collect_response_data;
  client_config.user_data = &collector;
  client_config.keep_alive_enable = false;
  if (request.endpoint.scheme == wfrac_transport::TransportScheme::https) {
    client_config.cert_pem = config_.tls.server_cert_pem;
    client_config.cert_len = config_.tls.esp_idf_cert_len();
    if (is_nonempty(config_.tls.common_name)) {
      client_config.common_name = config_.tls.common_name;
    }
  }

  esp_http_client_handle_t client = esp_http_client_init(&client_config);
  if (client == nullptr) return TransportError::io_failure;

  if (esp_http_client_set_header(client, "Content-Type", "application/json") != ESP_OK ||
      esp_http_client_set_header(client, "Connection", "close") != ESP_OK ||
      (request.body.size != 0 &&
       esp_http_client_set_post_field(client, reinterpret_cast<const char*>(request.body.data),
                                      static_cast<int>(request.body.size)) != ESP_OK)) {
    esp_http_client_cleanup(client);
    return TransportError::io_failure;
  }

  const esp_err_t perform_error = esp_http_client_perform(client);
  if (perform_error != ESP_OK) {
    int mbedtls_error = 0;
    int tls_verify_flags = 0;
    const esp_err_t tls_error = esp_http_client_get_and_clear_last_tls_error(
        client, &mbedtls_error, &tls_verify_flags);
    const EspHttpFailureEvidence evidence{perform_error, esp_http_client_get_errno(client), tls_error,
                                          mbedtls_error, tls_verify_flags};
    esp_http_client_cleanup(client);
    return classify_esp_http_failure(evidence);
  }

  response->http_status = esp_http_client_get_status_code(client);
  const TransportError result = collector.overflow ? TransportError::response_too_large : TransportError::none;
  response->body_size = collector.overflow ? 0 : collector.written;
  esp_http_client_cleanup(client);
  return result;
}

}  // namespace wfrac_transport_espidf
