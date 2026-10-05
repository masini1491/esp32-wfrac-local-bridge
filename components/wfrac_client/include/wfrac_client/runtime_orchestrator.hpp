#pragma once

#include <array>
#include <cstdint>

#include "wfrac_client/control_verification.hpp"
#include "wfrac_device_model/protocol_request_mapping.hpp"

namespace wfrac_client {

// Scheduling time is monotonic, not the wall-clock Unix time used by API builders.
class IMonotonicClock {
 public:
  virtual ~IMonotonicClock() = default;
  virtual uint64_t now_ms() = 0;
};

// Caller policy, NOT measured WF-RAC timing. Zero settle delay is permitted;
// every other interval/limit must be positive. No device timing is inferred.
struct RuntimePolicy {
  uint64_t poll_interval_ms = 0;
  uint64_t poll_retry_interval_ms = 0;
  uint64_t verification_delay_ms = 0;
  uint64_t readback_interval_ms = 0;
  uint64_t command_deadline_ms = 0;
  uint64_t max_baseline_age_ms = 0;
  std::size_t max_poll_attempts = 0;
  std::size_t max_readback_attempts = 0;
  std::size_t availability_failure_threshold = 0;
  std::size_t queue_capacity = 0;
};

// All pointers, strings and five distinct buffers are non-owning. Keep them
// valid for the initialized orchestrator's lifetime. Do not mutate DeviceState,
// SchemeCache or buffers behind it; serialize access externally. No OS owner.
struct RuntimeBindings {
  wfrac_api::ApiClientContext api_context{};
  const char* aircon_id = nullptr;
  wfrac_api::ITimeProvider* time_provider = nullptr;
  IMonotonicClock* clock = nullptr;
  ClientEndpoint endpoint{};
  wfrac_transport::SchemeCache* scheme_cache = nullptr;
  wfrac_transport::ITransport* transport = nullptr;
  wfrac_device_model::DeviceState* state = nullptr;
  PostWriteVerificationBuffers buffers{};
  wfrac_protocol::MutableByteView control_raw{};
};

enum class RuntimeError { none, invalid_argument, busy, queue_full, clock_regressed, state_conflict };
enum class RuntimeAction { idle, poll, write, readback, expire_command, discard_satisfied };
enum class RuntimeOutcome { idle, observation, accepted, verifying, confirmed, not_confirmed,
                            failed, timed_out };

struct RuntimeStepReport {
  RuntimeError error = RuntimeError::none;
  RuntimeAction action = RuntimeAction::idle;
  RuntimeOutcome outcome = RuntimeOutcome::idle;
  wfrac_api::ApiRequestError request_error = wfrac_api::ApiRequestError::none;
  wfrac_transport::TransportError transport_error = wfrac_transport::TransportError::none;
  wfrac_api::ApiResponseError response_error = wfrac_api::ApiResponseError::none;
  wfrac_device_model::ObservationMappingError observation_error =
      wfrac_device_model::ObservationMappingError::none;
  wfrac_device_model::ProtocolRequestMappingError mapping_error =
      wfrac_device_model::ProtocolRequestMappingError::none;
  ControlVerificationReport verification{};
  bool poll_retry_scheduled = false;
};

// Deterministic synchronous step core, not a scheduler/thread/task. Each step
// performs at most ONE logical API operation (executor SchemePolicy may allow
// at most two physical attempts). Writes are NEVER retried. Verification uses
// the existing verifier; confirmation means observation convergence, not causality
// or physical actuation. Callers choose when to step; no autonomous execution.
// Deadlines are checked before/after synchronous calls, not preemptive; late
// observations remain authoritative but report timed_out, not on-time confirmed.
// A monotonic regression latches a fault; recovery belongs to the caller.
class RuntimeOrchestrator final {
 public:
  static constexpr std::size_t max_queue_capacity = 4U;
  RuntimeOrchestrator() = default;
  RuntimeOrchestrator(const RuntimeOrchestrator&) = delete;
  RuntimeOrchestrator& operator=(const RuntimeOrchestrator&) = delete;
  RuntimeError initialize(RuntimeBindings bindings, RuntimePolicy policy);
  RuntimeError enqueue(const wfrac_device_model::CommandIntent& intent);
  RuntimeStepReport step();
  std::size_t queued_commands() const { return queued_; }
  bool verifying() const { return session_.write_accepted; }

 private:
  void pop();
  void record_read(bool success);
  void poll(RuntimeStepReport* report);
  bool desired_present(const wfrac_device_model::CommandIntent& intent) const;
  RuntimeBindings bindings_{};
  RuntimePolicy policy_{};
  std::array<wfrac_device_model::CommandIntent, max_queue_capacity> queue_{};
  std::size_t queued_ = 0;
  std::size_t baseline_size_ = 0;
  std::size_t poll_attempts_ = 0;
  std::size_t read_failures_ = 0;
  uint64_t last_now_ = 0;
  uint64_t baseline_at_ = 0;
  uint64_t next_poll_ = 0;
  uint64_t next_readback_ = 0;
  uint64_t command_deadline_ = 0;
  PostWriteVerificationSession session_{};
  bool initialized_ = false;
  bool executing_ = false;
  bool clock_fault_ = false;
};

}  // namespace wfrac_client
