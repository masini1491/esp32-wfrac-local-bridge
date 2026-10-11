#include "wfrac_client/runtime_orchestrator.hpp"

#include <cstring>
#include <limits>

namespace wfrac_client {
namespace {

bool nonempty(const char* value) { return value != nullptr && value[0] != '\0'; }
bool response_identity_matches(const wfrac_api::GetAirconStatContents& contents,
                               wfrac_protocol::MutableByteView identity_buffer,
                               const char* requested_aircon_id) {
  if (!contents.has_aircon_id) return true;
  const std::size_t requested_size = std::strlen(requested_aircon_id);
  return contents.aircon_id_size == requested_size &&
         std::memcmp(identity_buffer.data, requested_aircon_id, requested_size) == 0;
}
uint64_t after(uint64_t now, uint64_t delay) {
  return delay > std::numeric_limits<uint64_t>::max() - now
             ? std::numeric_limits<uint64_t>::max() : now + delay;
}
bool transient_read_error(wfrac_transport::TransportError error) {
  using E = wfrac_transport::TransportError;
  return error == E::timeout || error == E::connection_failure || error == E::io_failure;
}
bool valid_buffers(const RuntimeBindings& b) {
  const wfrac_protocol::MutableByteView buffers[] = {
      b.buffers.request_buffer, {b.buffers.response_buffer.data, b.buffers.response_buffer.size},
      b.buffers.base64_scratch, b.buffers.raw_aircon_stat, b.control_raw};
  for (std::size_t i = 0; i < 5U; ++i) {
    if (buffers[i].data == nullptr || buffers[i].size == 0U) return false;
    const auto start = reinterpret_cast<uintptr_t>(buffers[i].data);
    if (buffers[i].size > std::numeric_limits<uintptr_t>::max() - start) return false;
    for (std::size_t j = 0; j < i; ++j) {
      const auto other = reinterpret_cast<uintptr_t>(buffers[j].data);
      if (start < other + buffers[j].size && other < start + buffers[i].size) return false;
    }
  }
  return true;
}

}  // namespace

RuntimeError RuntimeOrchestrator::initialize(RuntimeBindings b, RuntimePolicy p) {
  if (initialized_ || executing_) return RuntimeError::busy;
  if (!nonempty(b.api_context.api_version) || !nonempty(b.api_context.device_id) ||
      !nonempty(b.api_context.operator_id) || !nonempty(b.aircon_id) || b.time_provider == nullptr ||
      b.clock == nullptr || !nonempty(b.endpoint.host) || b.endpoint.port == 0 ||
      b.endpoint.timeout_ms == 0 || b.scheme_cache == nullptr || b.transport == nullptr ||
      b.state == nullptr || !valid_buffers(b) || p.poll_interval_ms == 0 ||
      p.poll_retry_interval_ms == 0 || p.readback_interval_ms == 0 || p.command_deadline_ms == 0 ||
      p.max_baseline_age_ms == 0 || p.max_poll_attempts == 0 || p.max_readback_attempts == 0 ||
      p.availability_failure_threshold == 0 || p.queue_capacity == 0 ||
      p.queue_capacity > max_queue_capacity) return RuntimeError::invalid_argument;
  if (b.state->pending.intent.has_value() || b.state->pending.phase !=
      wfrac_device_model::PendingCommandPhase::none) return RuntimeError::state_conflict;
  bindings_ = b;
  policy_ = p;
  last_now_ = b.clock->now_ms();
  next_poll_ = last_now_;
  initialized_ = true;
  return RuntimeError::none;
}

RuntimeError RuntimeOrchestrator::enqueue(const wfrac_device_model::CommandIntent& intent) {
  if (!initialized_) return RuntimeError::invalid_argument;
  if (executing_) return RuntimeError::busy;
  // Validate using the existing semantic mapper, without mutating desired on error.
  wfrac_protocol::CommandState command{};
  if (wfrac_device_model::map_command_intent_to_protocol(intent, &command) !=
      wfrac_device_model::ProtocolCommandMappingError::none) return RuntimeError::invalid_argument;
  std::size_t slot = queued_;
  for (std::size_t i = 0; i < queued_; ++i) {
    if (queue_[i].index() == intent.index()) { slot = i; break; }
  }
  if (slot == queued_ && queued_ == policy_.queue_capacity) return RuntimeError::queue_full;
  if (wfrac_device_model::stage_desired_intent(intent, bindings_.state) !=
      wfrac_device_model::DeviceStateError::none) return RuntimeError::invalid_argument;
  queue_[slot] = intent;
  if (slot == queued_) ++queued_;
  return RuntimeError::none;
}

void RuntimeOrchestrator::pop() {
  for (std::size_t i = 1; i < queued_; ++i) queue_[i - 1] = queue_[i];
  --queued_;
}

bool RuntimeOrchestrator::desired_present(const wfrac_device_model::CommandIntent& intent) const {
  const auto& desired = bindings_.state->desired;
  return std::visit([&desired](const auto& value) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, wfrac_device_model::SetPowerIntent>) return desired.power.has_value();
    else if constexpr (std::is_same_v<T, wfrac_device_model::SetModeIntent>) return desired.mode.has_value();
    else if constexpr (std::is_same_v<T, wfrac_device_model::SetFanIntent>) return desired.fan.has_value();
    else return desired.target_temperature_centi_celsius.has_value();
  }, intent);
}

void RuntimeOrchestrator::record_read(bool success) {
  if (success) {
    read_failures_ = 0;
    wfrac_device_model::set_availability(wfrac_device_model::AvailabilityState::available, bindings_.state);
  } else {
    if (read_failures_ < policy_.availability_failure_threshold) ++read_failures_;
    if (read_failures_ == policy_.availability_failure_threshold)
      wfrac_device_model::set_availability(wfrac_device_model::AvailabilityState::unavailable, bindings_.state);
  }
}

void RuntimeOrchestrator::poll(RuntimeStepReport* r) {
  r->action = RuntimeAction::poll;
  r->outcome = RuntimeOutcome::failed;
  baseline_size_ = 0;  // The parser may overwrite raw storage even on failure.
  wfrac_api::ApiRequestView request{};
  r->request_error = wfrac_api::build_get_aircon_stat_request(
      bindings_.api_context, bindings_.aircon_id, bindings_.time_provider,
      bindings_.buffers.request_buffer, &request);
  if (r->request_error != wfrac_api::ApiRequestError::none) return;
  ++poll_attempts_;
  wfrac_transport::TransportResponse response{};
  r->transport_error = execute_api_request(request, bindings_.endpoint, bindings_.scheme_cache,
      bindings_.transport, bindings_.buffers.response_buffer, &response);
  if (r->transport_error != wfrac_transport::TransportError::none) {
    record_read(false);
    r->poll_retry_scheduled = transient_read_error(r->transport_error) &&
                             poll_attempts_ < policy_.max_poll_attempts;
    return;
  }
  wfrac_api::ApiResponseEnvelopeView envelope{};
  r->response_error = wfrac_api::parse_api_response_envelope(
      {bindings_.buffers.response_buffer.data, response.body_size}, "getAirconStat", &envelope);
  if (r->response_error != wfrac_api::ApiResponseError::none || envelope.result != 0 || !envelope.has_contents) {
    record_read(false);
    return;
  }
  wfrac_api::GetAirconStatContents contents{};
  r->response_error = wfrac_api::parse_get_aircon_stat_contents(envelope.contents_json,
      bindings_.buffers.request_buffer,
      bindings_.buffers.base64_scratch, bindings_.buffers.raw_aircon_stat, &contents);
  if (r->response_error != wfrac_api::ApiResponseError::none) { record_read(false); return; }
  if (!response_identity_matches(contents, bindings_.buffers.request_buffer, bindings_.aircon_id)) {
    r->error = RuntimeError::response_identity_mismatch;
    record_read(false);
    return;
  }
  r->observation_error = wfrac_device_model::reconcile_aircon_frame_observation(contents.frame, bindings_.state).error;
  if (r->observation_error != wfrac_device_model::ObservationMappingError::none) { record_read(false); return; }
  baseline_size_ = contents.raw_aircon_stat_size;
  record_read(true);
  r->outcome = RuntimeOutcome::observation;
}

RuntimeStepReport RuntimeOrchestrator::step() {
  RuntimeStepReport r{};
  if (!initialized_) { r.error = RuntimeError::invalid_argument; return r; }
  if (executing_) { r.error = RuntimeError::busy; return r; }
  const auto now = bindings_.clock->now_ms();
  if (clock_fault_ || now < last_now_) {
    clock_fault_ = true;
    r.error = RuntimeError::clock_regressed;
    return r;
  }
  last_now_ = now;
  if (bindings_.state->pending.intent.has_value() != session_.write_accepted ||
      (session_.write_accepted && bindings_.state->pending.phase !=
       wfrac_device_model::PendingCommandPhase::verifying)) {
    r.error = RuntimeError::state_conflict;
    return r;
  }
  executing_ = true;
  bool operation_deadline = false;
  if (session_.write_accepted) {
    if (now >= command_deadline_) {
      r.action = RuntimeAction::expire_command;
      r.outcome = RuntimeOutcome::timed_out;
      (void)wfrac_device_model::finish_pending_command(
          wfrac_device_model::PendingCommandOutcome::timed_out, bindings_.state);
      session_ = {};
    } else if (now >= next_readback_) {
      operation_deadline = true;
      r.action = RuntimeAction::readback;
      baseline_size_ = 0;
      const auto outcome = perform_post_write_readback(bindings_.api_context, bindings_.aircon_id,
          bindings_.time_provider, bindings_.endpoint, bindings_.scheme_cache, bindings_.transport,
          bindings_.buffers, bindings_.state, &session_, &r.verification);
      baseline_size_ = r.verification.validated_raw_aircon_stat_size;
      if (baseline_size_ != 0) record_read(true);
      else if (outcome != ControlVerificationOutcome::readback_request_failure) record_read(false);
      r.outcome = outcome == ControlVerificationOutcome::confirmed ? RuntimeOutcome::confirmed :
                  outcome == ControlVerificationOutcome::verifying ? RuntimeOutcome::verifying :
                  outcome == ControlVerificationOutcome::not_confirmed ? RuntimeOutcome::not_confirmed :
                  RuntimeOutcome::failed;
      if (outcome != ControlVerificationOutcome::verifying) session_ = {};
    }
  } else if (queued_ != 0 && !desired_present(queue_[0])) {
    r.action = RuntimeAction::discard_satisfied;
    pop();
  } else if (queued_ != 0 && baseline_size_ != 0 && now - baseline_at_ <= policy_.max_baseline_age_ms) {
    r.action = RuntimeAction::write;
    r.outcome = RuntimeOutcome::failed;
    wfrac_protocol::AirconFrameView frame{};
    std::size_t size = 0;
    const auto intent = queue_[0];
    pop();  // A dispatched write is consumed even if its outcome is ambiguous.
    if (wfrac_protocol::parse_aircon_stat({bindings_.buffers.raw_aircon_stat.data, baseline_size_}, &frame) !=
        wfrac_protocol::FrameError::none || !frame.command.crc_valid || !frame.receive.crc_valid) {
      baseline_size_ = 0;
      r.mapping_error = wfrac_device_model::ProtocolRequestMappingError::invalid_receive_baseline;
    } else {
      r.mapping_error = wfrac_device_model::build_control_aircon_stat(intent, frame.receive, bindings_.control_raw, &size);
      if (r.mapping_error == wfrac_device_model::ProtocolRequestMappingError::none) {
        baseline_size_ = 0;  // A possibly delivered write invalidates the old compatibility baseline.
        operation_deadline = true;
        command_deadline_ = after(now, policy_.command_deadline_ms);
        const auto outcome = begin_set_aircon_stat_verification(intent, {bindings_.control_raw.data, size},
            bindings_.api_context, bindings_.aircon_id, bindings_.time_provider, bindings_.endpoint,
            bindings_.scheme_cache, bindings_.transport, {policy_.max_readback_attempts}, bindings_.buffers,
            bindings_.state, &session_, &r.verification);
        if (outcome == ControlVerificationOutcome::accepted) r.outcome = RuntimeOutcome::accepted;
      }
    }
  } else if (now >= next_poll_) {
    poll(&r);
  }
  const auto end = bindings_.clock->now_ms();
  if (end < now) {
    clock_fault_ = true;
    r.error = RuntimeError::clock_regressed;
  } else {
    last_now_ = end;
    if (baseline_size_ != 0 && (r.outcome == RuntimeOutcome::observation || r.action == RuntimeAction::readback))
      baseline_at_ = end;
    if (r.action == RuntimeAction::poll) {
      next_poll_ = after(end, r.poll_retry_scheduled ? policy_.poll_retry_interval_ms : policy_.poll_interval_ms);
      if (!r.poll_retry_scheduled) poll_attempts_ = 0;
    }
    if (r.action == RuntimeAction::readback || r.action == RuntimeAction::write) {
      next_readback_ = after(end, r.action == RuntimeAction::write ? policy_.verification_delay_ms : policy_.readback_interval_ms);
      next_poll_ = after(end, policy_.poll_interval_ms);
    }
    if (operation_deadline && end >= command_deadline_) {
      (void)wfrac_device_model::finish_pending_command(
          wfrac_device_model::PendingCommandOutcome::timed_out, bindings_.state);
      session_ = {};
      r.outcome = RuntimeOutcome::timed_out;
    }
  }
  executing_ = false;
  return r;
}

}  // namespace wfrac_client
