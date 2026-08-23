#include "wfrac_transport_espidf/error_classifier.hpp"

using wfrac_transport::TransportError;
using wfrac_transport_espidf::EspHttpFailureEvidence;
using wfrac_transport_espidf::classify_esp_http_failure;

static_assert(classify_esp_http_failure({ESP_OK, 0, ESP_FAIL, -1, 1}) == TransportError::none);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_OK, 0, 1}) == TransportError::tls_identity_failure);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_FAIL}) == TransportError::tls_failure);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_READ_TIMEOUT}) == TransportError::timeout);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_INVALID_TRANSPORT}) == TransportError::invalid_argument);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_CONNECT}) != TransportError::scheme_mismatch);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_OK, 0, 1}) != TransportError::scheme_mismatch);

extern "C" void app_main(void) {}
