#include "wfrac_client/control_verification.hpp"

#include <cstring>

namespace wfrac_client {
namespace {

bool is_nonempty(const char* value) { return value != nullptr && value[0] != '\0'; }

bool is_valid_buffer(wfrac_protocol::MutableByteView buffer) {
  return buffer.data != nullptr && buffer.size != 0U;
}

bool is_valid_buffer(wfrac_transport::MutableByteView buffer) {
  return buffer.data != nullptr && buffer.size != 0U;
}

bool response_identity_matches(const wfrac_api::GetAirconStatContents& contents,
                               wfrac_protocol::MutableByteView identity_buffer,
                               const char* requested_aircon_id) {
  if (!contents.has_aircon_id) return true;
  const std::size_t requested_size = std::strlen(requested_aircon_id);
  return contents.aircon_id_size == requested_size &&
         std::memcmp(identity_buffer.data, requested_aircon_id, requested_size) == 0;
}

bool is_valid_common_input(const wfrac_api::ApiClientContext& context, const char* aircon_id,
                           wfrac_api::ITimeProvider* time_provider, const ClientEndpoint& endpoint,
                           wfrac_transport::SchemeCache* scheme_cache,
                           wfrac_transport::ITransport* transport,
                           const PostWriteVerificationBuffers& buffers,
                           wfrac_device_model::DeviceState* state,
                           PostWriteVerificationSession* session,
                           ControlVerificationReport* report) {
  return is_nonempty(context.api_version) && is_nonempty(context.device_id) &&
         is_nonempty(context.operator_id) && is_nonempty(aircon_id) && time_provider != nullptr &&
         is_nonempty(endpoint.host) && endpoint.port != 0U && endpoint.timeout_ms != 0U &&
         scheme_cache != nullptr && transport != nullptr && is_valid_buffer(buffers.request_buffer) &&
         is_valid_buffer(buffers.response_buffer) && is_valid_buffer(buffers.base64_scratch) &&
         is_valid_buffer(buffers.raw_aircon_stat) && state != nullptr && session != nullptr && report != nullptr;
}

void reset_report(ControlVerificationReport* report) {
  if (report != nullptr) *report = {};
}

void finish(wfrac_device_model::PendingCommandOutcome outcome, wfrac_device_model::DeviceState* state) {
  (void)wfrac_device_model::finish_pending_command(outcome, state);
}

ControlVerificationOutcome transport_failure(wfrac_transport::TransportError error,
                                             bool is_write,
                                             wfrac_device_model::DeviceState* state,
                                             ControlVerificationReport* report) {
  report->transport_error = error;
  const auto pending_outcome = error == wfrac_transport::TransportError::timeout
                                   ? wfrac_device_model::PendingCommandOutcome::timed_out
                                   : wfrac_device_model::PendingCommandOutcome::failed;
  finish(pending_outcome, state);
  report->outcome = is_write ? ControlVerificationOutcome::write_transport_failure
                             : ControlVerificationOutcome::readback_transport_failure;
  return report->outcome;
}

}  // namespace

ControlVerificationOutcome begin_set_aircon_stat_verification(
    const wfrac_device_model::CommandIntent& intent, wfrac_protocol::ByteView raw_aircon_stat,
    const wfrac_api::ApiClientContext& api_context, const char* aircon_id,
    wfrac_api::ITimeProvider* time_provider, const ClientEndpoint& endpoint,
    wfrac_transport::SchemeCache* scheme_cache, wfrac_transport::ITransport* transport,
    PostWriteVerificationPolicy policy, PostWriteVerificationBuffers buffers,
    wfrac_device_model::DeviceState* state, PostWriteVerificationSession* session,
    ControlVerificationReport* report) {
  reset_report(report);
  if (!is_valid_common_input(api_context, aircon_id, time_provider, endpoint, scheme_cache, transport, buffers,
                             state, session, report) ||
      raw_aircon_stat.data == nullptr || raw_aircon_stat.size == 0U || policy.max_readback_attempts == 0U) {
    if (report != nullptr) report->outcome = ControlVerificationOutcome::invalid_argument;
    return ControlVerificationOutcome::invalid_argument;
  }
  *session = {};

  if (wfrac_device_model::begin_pending_command(intent, state) != wfrac_device_model::DeviceStateError::none) {
    report->outcome = ControlVerificationOutcome::state_error;
    return report->outcome;
  }
  if (wfrac_device_model::stage_desired_intent(intent, state) != wfrac_device_model::DeviceStateError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::state_error;
    return report->outcome;
  }

  wfrac_api::ApiRequestView write_request{};
  if (wfrac_api::build_set_aircon_stat_request(api_context, aircon_id, raw_aircon_stat, time_provider,
                                               buffers.request_buffer, &write_request) !=
      wfrac_api::ApiRequestError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::write_request_failure;
    return report->outcome;
  }

  wfrac_transport::TransportResponse response{};
  const auto transport_error = execute_api_request(write_request, endpoint, scheme_cache, transport,
                                                    buffers.response_buffer, &response);
  if (transport_error != wfrac_transport::TransportError::none) {
    return transport_failure(transport_error, true, state, report);
  }

  wfrac_api::ApiResponseEnvelopeView envelope{};
  report->response_error = wfrac_api::parse_api_response_envelope(
      {buffers.response_buffer.data, response.body_size}, "setAirconStat", &envelope);
  if (report->response_error != wfrac_api::ApiResponseError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::write_response_failure;
    return report->outcome;
  }
  report->application_result = envelope.result;
  if (envelope.result != 0) {
    finish(wfrac_device_model::PendingCommandOutcome::rejected, state);
    report->outcome = ControlVerificationOutcome::write_rejected;
    return report->outcome;
  }

  if (wfrac_device_model::mark_pending_command_accepted(state) != wfrac_device_model::DeviceStateError::none ||
      wfrac_device_model::begin_pending_verification(state) != wfrac_device_model::DeviceStateError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::state_error;
    return report->outcome;
  }
  session->write_accepted = true;
  session->max_readback_attempts = policy.max_readback_attempts;
  report->outcome = ControlVerificationOutcome::accepted;
  return report->outcome;
}

ControlVerificationOutcome perform_post_write_readback(
    const wfrac_api::ApiClientContext& api_context, const char* aircon_id,
    wfrac_api::ITimeProvider* time_provider, const ClientEndpoint& endpoint,
    wfrac_transport::SchemeCache* scheme_cache, wfrac_transport::ITransport* transport,
    PostWriteVerificationBuffers buffers, wfrac_device_model::DeviceState* state,
    PostWriteVerificationSession* session, ControlVerificationReport* report) {
  reset_report(report);
  if (!is_valid_common_input(api_context, aircon_id, time_provider, endpoint, scheme_cache, transport, buffers,
                             state, session, report) ||
      !session->write_accepted || session->max_readback_attempts == 0U ||
      session->readback_attempts >= session->max_readback_attempts || !state->pending.intent.has_value() ||
      state->pending.phase != wfrac_device_model::PendingCommandPhase::verifying) {
    if (report != nullptr) report->outcome = ControlVerificationOutcome::invalid_argument;
    return ControlVerificationOutcome::invalid_argument;
  }

  wfrac_api::ApiRequestView read_request{};
  if (wfrac_api::build_get_aircon_stat_request(api_context, aircon_id, time_provider, buffers.request_buffer,
                                               &read_request) != wfrac_api::ApiRequestError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::readback_request_failure;
    return report->outcome;
  }
  ++session->readback_attempts;

  wfrac_transport::TransportResponse response{};
  const auto transport_error = execute_api_request(read_request, endpoint, scheme_cache, transport,
                                                    buffers.response_buffer, &response);
  if (transport_error != wfrac_transport::TransportError::none) {
    return transport_failure(transport_error, false, state, report);
  }

  wfrac_api::ApiResponseEnvelopeView envelope{};
  report->response_error = wfrac_api::parse_api_response_envelope(
      {buffers.response_buffer.data, response.body_size}, "getAirconStat", &envelope);
  if (report->response_error != wfrac_api::ApiResponseError::none || envelope.result != 0 ||
      !envelope.has_contents) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->application_result = envelope.result;
    report->outcome = ControlVerificationOutcome::readback_response_failure;
    return report->outcome;
  }
  report->application_result = envelope.result;

  wfrac_api::GetAirconStatContents contents{};
  report->response_error = wfrac_api::parse_get_aircon_stat_contents(
      envelope.contents_json, buffers.request_buffer, buffers.base64_scratch, buffers.raw_aircon_stat, &contents);
  if (report->response_error != wfrac_api::ApiResponseError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::readback_response_failure;
    return report->outcome;
  }
  if (!response_identity_matches(contents, buffers.request_buffer, aircon_id)) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    session->write_accepted = false;
    report->outcome = ControlVerificationOutcome::readback_identity_mismatch;
    return report->outcome;
  }

  const auto reconciled = wfrac_device_model::reconcile_aircon_frame_observation(contents.frame, state);
  report->observation_error = reconciled.error;
  if (reconciled.error != wfrac_device_model::ObservationMappingError::none) {
    finish(wfrac_device_model::PendingCommandOutcome::failed, state);
    report->outcome = ControlVerificationOutcome::readback_observation_failure;
    return report->outcome;
  }
  report->validated_raw_aircon_stat_size = contents.raw_aircon_stat_size;
  if (reconciled.reconciliation.pending == wfrac_device_model::PendingReconciliation::observed_converged) {
    session->write_accepted = false;
    report->outcome = ControlVerificationOutcome::confirmed;
    return report->outcome;
  }

  session->conflicting_observation_seen = true;
  if (session->readback_attempts == session->max_readback_attempts) {
    finish(wfrac_device_model::PendingCommandOutcome::not_confirmed, state);
    session->write_accepted = false;
    report->outcome = ControlVerificationOutcome::not_confirmed;
    return report->outcome;
  }
  report->outcome = ControlVerificationOutcome::verifying;
  return report->outcome;
}

}  // namespace wfrac_client
