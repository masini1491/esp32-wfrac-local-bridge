#pragma once

#include <cstddef>

#include "wfrac_api/api_request.hpp"
#include "wfrac_api/api_response.hpp"
#include "wfrac_client/request_executor.hpp"
#include "wfrac_device_model/device_state.hpp"
#include "wfrac_device_model/protocol_observation.hpp"

namespace wfrac_client {

// The runtime owns scheduling between readback steps. This policy only bounds
// how many authoritative readbacks one accepted command may consume.
struct PostWriteVerificationPolicy {
  std::size_t max_readback_attempts = 1U;
};

// All buffers are caller-owned. They must remain valid for the corresponding
// synchronous begin/step call; base64_scratch and raw_aircon_stat are reused by
// the existing getAirconStat parser during each verification step.
struct PostWriteVerificationBuffers {
  wfrac_protocol::MutableByteView request_buffer{};
  wfrac_transport::MutableByteView response_buffer{};
  wfrac_protocol::MutableByteView base64_scratch{};
  wfrac_protocol::MutableByteView raw_aircon_stat{};
};

enum class ControlVerificationOutcome {
  invalid_argument,
  state_error,
  write_request_failure,
  write_transport_failure,
  write_response_failure,
  write_rejected,
  accepted,
  verifying,
  confirmed,
  not_confirmed,
  readback_request_failure,
  readback_transport_failure,
  readback_response_failure,
  readback_identity_mismatch,
  readback_observation_failure,
};

struct PostWriteVerificationSession {
  std::size_t readback_attempts = 0U;
  std::size_t max_readback_attempts = 0U;
  bool write_accepted = false;
  bool conflicting_observation_seen = false;
};

struct ControlVerificationReport {
  ControlVerificationOutcome outcome = ControlVerificationOutcome::invalid_argument;
  wfrac_transport::TransportError transport_error = wfrac_transport::TransportError::none;
  wfrac_api::ApiResponseError response_error = wfrac_api::ApiResponseError::none;
  wfrac_device_model::ObservationMappingError observation_error =
      wfrac_device_model::ObservationMappingError::none;
  int64_t application_result = 0;
  // Nonzero only after successful integrity parsing and observation mapping.
  // Bytes reside in buffers.raw_aircon_stat; no frame view is retained here.
  std::size_t validated_raw_aircon_stat_size = 0U;
};

// Begins a setAirconStat command. A result of accepted means only that the
// command response was accepted; callers must later invoke
// perform_post_write_readback() until it returns a terminal outcome.
ControlVerificationOutcome begin_set_aircon_stat_verification(
    const wfrac_device_model::CommandIntent& intent,
    wfrac_protocol::ByteView raw_aircon_stat,
    const wfrac_api::ApiClientContext& api_context,
    const char* aircon_id,
    wfrac_api::ITimeProvider* time_provider,
    const ClientEndpoint& endpoint,
    wfrac_transport::SchemeCache* scheme_cache,
    wfrac_transport::ITransport* transport,
    PostWriteVerificationPolicy policy,
    PostWriteVerificationBuffers buffers,
    wfrac_device_model::DeviceState* state,
    PostWriteVerificationSession* session,
    ControlVerificationReport* report);

// Performs one caller-scheduled getAirconStat readback. No write is retried.
// A mismatching frame updates DeviceState.observed through the existing
// reconciliation path. Its writer identity is not inferred by this API.
ControlVerificationOutcome perform_post_write_readback(
    const wfrac_api::ApiClientContext& api_context,
    const char* aircon_id,
    wfrac_api::ITimeProvider* time_provider,
    const ClientEndpoint& endpoint,
    wfrac_transport::SchemeCache* scheme_cache,
    wfrac_transport::ITransport* transport,
    PostWriteVerificationBuffers buffers,
    wfrac_device_model::DeviceState* state,
    PostWriteVerificationSession* session,
    ControlVerificationReport* report);

}  // namespace wfrac_client
