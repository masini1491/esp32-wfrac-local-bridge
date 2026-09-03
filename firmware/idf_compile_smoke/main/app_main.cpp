#include <type_traits>

#include "wfrac_transport_espidf/error_classifier.hpp"
#include "wfrac_transport_espidf/esp_http_transport.hpp"
#include "wfrac_client/control_verification.hpp"
#include "wfrac_client/request_executor.hpp"
#include "wfrac_api/api_response.hpp"
#include "wfrac_device_model/protocol_observation.hpp"
#include "wfrac_time_espidf/esp_system_time_provider.hpp"

using wfrac_transport::TransportError;
using wfrac_transport_espidf::EspHttpFailureEvidence;
using wfrac_transport_espidf::classify_esp_http_failure;

constexpr char kPem[] = "-----BEGIN CERTIFICATE-----\nX\n-----END CERTIFICATE-----\n";
constexpr char kEmptyPem[] = "";
constexpr char kPemWithoutTerminator[] = {'P', 'E', 'M'};
constexpr char kPemWithEmbeddedNul[] = {'P', '\0', 'E', 'M', '\0'};

static_assert(classify_esp_http_failure({ESP_OK, 0, ESP_FAIL, -1, 1}) == TransportError::none);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_OK, 0, 1}) == TransportError::tls_identity_failure);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_FAIL}) == TransportError::tls_failure);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_READ_TIMEOUT}) == TransportError::timeout);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT}) ==
              TransportError::timeout);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME}) ==
              TransportError::connection_failure);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET}) ==
              TransportError::connection_failure);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST}) ==
              TransportError::connection_failure);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED}) ==
              TransportError::tls_failure);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_INVALID_TRANSPORT, 0, ESP_FAIL}) ==
              TransportError::invalid_argument);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_INVALID_TRANSPORT}) == TransportError::invalid_argument);
static_assert(classify_esp_http_failure({ESP_ERR_HTTP_CONNECT}) != TransportError::scheme_mismatch);
static_assert(classify_esp_http_failure({ESP_FAIL, 0, ESP_OK, 0, 1}) != TransportError::scheme_mismatch);
static_assert(wfrac_transport_espidf::EspTlsVerificationConfig{kPem, 0U, nullptr}.has_valid_pem_certificate());
static_assert(wfrac_transport_espidf::EspTlsVerificationConfig{kPem, sizeof(kPem), nullptr}
                  .has_valid_pem_certificate());
static_assert(wfrac_transport_espidf::EspTlsVerificationConfig{kPem, sizeof(kPem), nullptr}.esp_idf_cert_len() ==
              0U);
static_assert(!wfrac_transport_espidf::EspTlsVerificationConfig{kPemWithoutTerminator,
                                                                 sizeof(kPemWithoutTerminator), nullptr}
                   .has_valid_pem_certificate());
static_assert(!wfrac_transport_espidf::EspTlsVerificationConfig{kPemWithEmbeddedNul,
                                                                 sizeof(kPemWithEmbeddedNul), nullptr}
                   .has_valid_pem_certificate());
static_assert(!wfrac_transport_espidf::EspTlsVerificationConfig{nullptr, 0U, nullptr}.has_valid_pem_certificate());
static_assert(!wfrac_transport_espidf::EspTlsVerificationConfig{kEmptyPem, 0U, nullptr}.has_valid_pem_certificate());
static_assert(std::is_base_of_v<wfrac_transport::ITransport, wfrac_transport_espidf::EspHttpTransport>);
static_assert(sizeof(wfrac_client::RequestExecutionReport) >= sizeof(std::size_t));
static_assert(sizeof(wfrac_client::PostWriteVerificationSession) >= sizeof(std::size_t));
static_assert(sizeof(wfrac_api::ApiResponseEnvelopeView) >= sizeof(int64_t));
static_assert(sizeof(wfrac_api::GetAirconStatContents) >= sizeof(std::size_t));
static_assert(sizeof(wfrac_device_model::ObservationReconciliationResult) >= sizeof(std::size_t));
static_assert(std::is_base_of_v<wfrac_api::ITimeProvider, wfrac_time_espidf::EspSystemTimeProvider>);

extern "C" void app_main(void) {}
