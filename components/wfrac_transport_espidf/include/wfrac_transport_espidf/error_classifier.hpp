#pragma once

#include <cerrno>

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_tls_errors.h"

#include "wfrac_transport/transport.hpp"

namespace wfrac_transport_espidf {

// Adapter-specific evidence captured for one completed ESP-IDF HTTP attempt.
// It is diagnostic input only; it does not select a scheme or request fallback.
struct EspHttpFailureEvidence {
  esp_err_t perform_error = ESP_OK;
  int socket_errno = 0;
  esp_err_t tls_error = ESP_OK;
  int mbedtls_error = 0;
  int tls_verify_flags = 0;
};

constexpr wfrac_transport::TransportError classify_esp_http_failure(const EspHttpFailureEvidence& evidence) {
  using wfrac_transport::TransportError;

  if (evidence.perform_error == ESP_OK) return TransportError::none;
  if (evidence.tls_verify_flags != 0) return TransportError::tls_identity_failure;
  if (evidence.perform_error == ESP_ERR_HTTP_CONNECTING || evidence.perform_error == ESP_ERR_HTTP_EAGAIN ||
      evidence.perform_error == ESP_ERR_HTTP_READ_TIMEOUT ||
      evidence.tls_error == ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT ||
      evidence.tls_error == ESP_ERR_ESP_TLS_SERVER_HANDSHAKE_TIMEOUT || evidence.socket_errno == ETIMEDOUT ||
      evidence.socket_errno == EAGAIN || evidence.socket_errno == EWOULDBLOCK) {
    return TransportError::timeout;
  }
  if (evidence.perform_error == ESP_ERR_HTTP_CONNECT || evidence.socket_errno == ECONNREFUSED ||
      evidence.socket_errno == ENETUNREACH || evidence.socket_errno == EHOSTUNREACH ||
      evidence.tls_error == ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME ||
      evidence.tls_error == ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET ||
      evidence.tls_error == ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST) {
    return TransportError::connection_failure;
  }
  if (evidence.perform_error == ESP_ERR_INVALID_ARG || evidence.perform_error == ESP_ERR_HTTP_INVALID_TRANSPORT) {
    return TransportError::invalid_argument;
  }
  if (evidence.tls_error != ESP_OK || evidence.mbedtls_error != 0) return TransportError::tls_failure;
  return TransportError::io_failure;
}

}  // namespace wfrac_transport_espidf
