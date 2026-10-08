#include <type_traits>

#include "wfrac_transport_espidf/error_classifier.hpp"
#include "wfrac_transport_espidf/esp_http_transport.hpp"
#include "wfrac_client/control_verification.hpp"
#include "wfrac_client/request_executor.hpp"
#include "wfrac_client/runtime_orchestrator.hpp"
#include "wfrac_api/api_response.hpp"
#include "wfrac_device_model/protocol_observation.hpp"
#include "wfrac_device_model/extended_observation.hpp"
#include "wfrac_time_espidf/esp_system_time_provider.hpp"
#include "wfrac_matter_adapter/rac_mapping.hpp"
#include "wfrac_persistence_espidf/nvs_persistence.hpp"
#include "wfrac_protocol/aircon_state.hpp"
#include "wfrac_protocol/operation_data.hpp"

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
static_assert(std::is_default_constructible_v<wfrac_client::RuntimeOrchestrator>);
static_assert(wfrac_client::RuntimeOrchestrator::max_queue_capacity == 4U);
static_assert(sizeof(wfrac_api::ApiResponseEnvelopeView) >= sizeof(int64_t));
static_assert(sizeof(wfrac_api::GetAirconStatContents) >= sizeof(std::size_t));
static_assert(sizeof(wfrac_device_model::ObservationReconciliationResult) >= sizeof(std::size_t));
static_assert(sizeof(wfrac_device_model::ExtendedHvacObservation) >= sizeof(uint8_t));
static_assert(std::is_base_of_v<wfrac_api::ITimeProvider, wfrac_time_espidf::EspSystemTimeProvider>);

static_assert(wfrac_matter_adapter::room_air_conditioner_device_type == 0x0072U);
static_assert(wfrac_matter_adapter::room_air_conditioner_device_version == 3U);

// Link coverage only; never called by the inert app_main, no Matter SDK/runtime.
wfrac_matter_adapter::RacObservedAttributes matter_mapping_link_smoke() {
  wfrac_device_model::DeviceState state{};
  std::optional<wfrac_device_model::CommandIntent> intent;
  wfrac_matter_adapter::map_on_off_write(false, &intent);
  wfrac_matter_adapter::map_system_mode_write(0, state, &intent);
  wfrac_matter_adapter::map_setpoint_write(
      wfrac_matter_adapter::SetpointAttribute::occupied_cooling, 2350, state, &intent);
  return wfrac_matter_adapter::project_observed_attributes(state);
}

static_assert(std::is_base_of_v<wfrac_persistence::IPersistence, wfrac_persistence_espidf::NvsPersistence>);

// Compile/link coverage only: NOT called, no initialization or actual NVS access.
wfrac_persistence::PersistenceError persistence_link_smoke() {
  wfrac_persistence_espidf::NvsPersistence store;
  const wfrac_persistence::DeviceKey key{"synthetic"};
  char id[wfrac_persistence::max_operator_id_size + 1U]{};
  std::size_t size = 0;
  auto scheme = wfrac_transport::SchemeKnowledge::unknown;
  wfrac_persistence::TimeCheckpoint checkpoint{};
  store.load_operator_id(key, id, sizeof(id), &size);
  store.store_operator_id(key, {"synthetic", 9U});
  store.erase_operator_id(key);
  store.load_scheme(key, &scheme);
  store.store_scheme(key, wfrac_transport::SchemeKnowledge::https_confirmed);
  store.erase_scheme(key);
  store.load_time_checkpoint(&checkpoint);
  store.store_time_checkpoint({5000000000LL});
  return store.erase_time_checkpoint();
}

extern "C" void app_main(void) {}

// Compile/link coverage only; never called by app_main.
wfrac_device_model::ObservationMappingError extended_observation_link_smoke(
    const wfrac_protocol::AirconFrameView& frame,
    wfrac_device_model::ExtendedHvacObservation* observation) {
  return wfrac_device_model::map_aircon_frame_extended_observation(frame, observation);
}
// Pure decoder compile/link coverage; never called by the inert app_main.
wfrac_protocol::OperationValueError operation_value_link_smoke() {
  wfrac_protocol::AmbientTemperature ambient{};
  wfrac_protocol::CurrentRunEnergy energy{};
  wfrac_protocol::InferredCompressorFrequency frequency{};
  wfrac_protocol::InferredOperatingCurrent current{};
  wfrac_protocol::InferredDischargeTemperature temperature{};
  wfrac_protocol::InferredIndoorCoilTemperature indoor_coil{};
  wfrac_protocol::EevNetworkValue eev{};
  wfrac_protocol::OutdoorCoilRawValue outdoor_coil{};
  wfrac_protocol::TdshRawValue tdsh{};
  wfrac_protocol::decode_inferred_compressor_frequency({0x11U, 0x10U, 0, 0}, &frequency);
  wfrac_protocol::decode_inferred_operating_current({0x90U, 0x10U, 4U, 0}, &current);
  wfrac_protocol::decode_inferred_discharge_temperature({0x85U, 0x10U, 0x12U, 0}, &temperature);
  wfrac_protocol::decode_inferred_indoor_coil_temperature({0x81U, 0x20U, 170U, 0xA5U}, &indoor_coil);
  wfrac_protocol::decode_eev_network_value({0x13U, 0xA7U, 102U, 0x5CU}, &eev);
  wfrac_protocol::decode_outdoor_coil_raw_value({0x82U, 0xA5U, 0U, 0x5AU}, &outdoor_coil);
  wfrac_protocol::decode_tdsh_raw_value({0xB1U, 0xA5U, 0U, 0x5AU}, &tdsh);
  wfrac_protocol::decode_ambient_temperature({0x80U, 0x10U, 128U, 0}, &ambient);
  return wfrac_protocol::decode_current_run_energy({0x94U, 0x10U, 1U, 0}, &energy);
}

// Compile/link coverage only: never called by app_main; no protocol/runtime action.
wfrac_protocol::EncodeError vertical_vane_protocol_link_smoke() {
  wfrac_protocol::CommandState command{};
  const auto error = wfrac_protocol::encode_set_vertical_vane(
      wfrac_protocol::VerticalVaneCommand::position4, &command);
  const auto receive = wfrac_protocol::decode_vertical_direction(0x40U, 0x30U);
  return error == wfrac_protocol::EncodeError::none &&
                 receive.swing == wfrac_protocol::VerticalSwingState::swinging
             ? error
             : wfrac_protocol::EncodeError::invalid_vertical_vane;
}
