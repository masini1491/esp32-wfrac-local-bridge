#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "wfrac_client/control_verification.hpp"
#include "wfrac_protocol/base64.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {

struct Step {
  wfrac_transport::TransportError error = wfrac_transport::TransportError::none;
  const char* body = nullptr;
};

class ScriptedTransport final : public wfrac_transport::ITransport {
 public:
  ScriptedTransport(std::vector<Step> steps, const wfrac_device_model::DeviceState* state)
      : steps_(std::move(steps)), state_(state) {}

  wfrac_transport::TransportError perform(const wfrac_transport::TransportRequest& request,
                                           wfrac_transport::MutableByteView output,
                                           wfrac_transport::TransportResponse* response) override {
    assert(calls_ < steps_.size());
    assert(state_ != nullptr && state_->pending.intent.has_value());
    if (calls_ == 0U) {
      assert(state_->pending.phase == wfrac_device_model::PendingCommandPhase::sent);
      assert(std::strcmp(request.path, "/beaver/command/setAirconStat") == 0);
    } else {
      assert(state_->pending.phase == wfrac_device_model::PendingCommandPhase::verifying);
      assert(std::strcmp(request.path, "/beaver/command/getAirconStat") == 0);
    }
    const Step& step = steps_[calls_++];
    if (step.error != wfrac_transport::TransportError::none) return step.error;
    const std::size_t body_size = std::strlen(step.body);
    assert(body_size <= output.size);
    std::memcpy(output.data, step.body, body_size);
    response->http_status = 200;
    response->body_size = body_size;
    return wfrac_transport::TransportError::none;
  }

  std::size_t calls() const { return calls_; }

 private:
  std::vector<Step> steps_;
  const wfrac_device_model::DeviceState* state_ = nullptr;
  std::size_t calls_ = 0U;
};

class FakeTimeProvider final : public wfrac_api::ITimeProvider {
 public:
  wfrac_api::TimeSample now() override {
    ++calls;
    return {1700000000, wfrac_api::TimeQuality::trusted};
  }

  std::size_t calls = 0U;
};

std::array<uint8_t, 42> make_frame(bool power_on) {
  std::array<uint8_t, 42> raw{};
  raw[18] = 0U;
  const uint16_t command_crc = wfrac_protocol::crc16_ccitt(raw.data(), 19U);
  raw[19] = static_cast<uint8_t>(command_crc & 0xFFU);
  raw[20] = static_cast<uint8_t>(command_crc >> 8U);
  raw[23] = power_on ? 0x09U : 0x08U;
  raw[24] = 0x02U;
  raw[25] = 47U;
  raw[26] = 154U;
  raw[39] = 0U;
  const uint16_t receive_crc = wfrac_protocol::crc16_ccitt(raw.data() + 21U, 19U);
  raw[40] = static_cast<uint8_t>(receive_crc & 0xFFU);
  raw[41] = static_cast<uint8_t>(receive_crc >> 8U);
  return raw;
}

std::string get_response(const std::array<uint8_t, 42>& raw) {
  std::array<uint8_t, 56> encoded{};
  std::size_t encoded_size = 0U;
  assert(wfrac_protocol::base64_encode({raw.data(), raw.size()}, {encoded.data(), encoded.size()}, &encoded_size) ==
         wfrac_protocol::Base64Error::none);
  return std::string("{\"command\":\"getAirconStat\",\"result\":0,\"contents\":{\"airconStat\":\"") +
         std::string(reinterpret_cast<const char*>(encoded.data()), encoded_size) + "\"}}";
}

struct Fixture {
  std::array<uint8_t, 512> request{};
  std::array<uint8_t, 512> response{};
  std::array<uint8_t, 128> scratch{};
  std::array<uint8_t, 42> parsed_raw{};
  wfrac_client::PostWriteVerificationBuffers buffers{
      {request.data(), request.size()}, {response.data(), response.size()},
      {scratch.data(), scratch.size()}, {parsed_raw.data(), parsed_raw.size()}};
};

wfrac_client::ControlVerificationOutcome begin(
    const std::array<uint8_t, 42>& raw, FakeTimeProvider* time, ScriptedTransport* transport,
    wfrac_device_model::DeviceState* state, wfrac_client::PostWriteVerificationSession* session,
    wfrac_client::ControlVerificationReport* report, Fixture* fixture, std::size_t max_readbacks) {
  const wfrac_api::ApiClientContext context{"1.0", "synthetic-device", "synthetic-operator"};
  const wfrac_client::ClientEndpoint endpoint{"example.invalid", 51443U, 1000U};
  wfrac_transport::SchemeCache cache{wfrac_transport::SchemeKnowledge::http_confirmed};
  return wfrac_client::begin_set_aircon_stat_verification(
      wfrac_device_model::SetPowerIntent{wfrac_device_model::PowerState::on}, {raw.data(), raw.size()}, context,
      "synthetic-aircon", time, endpoint, &cache, transport, {max_readbacks}, fixture->buffers, state, session,
      report);
}

wfrac_client::ControlVerificationOutcome readback(
    FakeTimeProvider* time, ScriptedTransport* transport, wfrac_device_model::DeviceState* state,
    wfrac_client::PostWriteVerificationSession* session, wfrac_client::ControlVerificationReport* report,
    Fixture* fixture) {
  const wfrac_api::ApiClientContext context{"1.0", "synthetic-device", "synthetic-operator"};
  const wfrac_client::ClientEndpoint endpoint{"example.invalid", 51443U, 1000U};
  wfrac_transport::SchemeCache cache{wfrac_transport::SchemeKnowledge::http_confirmed};
  return wfrac_client::perform_post_write_readback(context, "synthetic-aircon", time, endpoint, &cache, transport,
                                                    fixture->buffers, state, session, report);
}

}  // namespace

int main() {
  using wfrac_client::ControlVerificationOutcome;
  const auto on_frame = make_frame(true);
  const auto off_frame = make_frame(false);
  const std::string read_on = get_response(on_frame);
  const std::string read_off = get_response(off_frame);
  constexpr char kWriteAccepted[] = "{\"command\":\"setAirconStat\",\"result\":0}";
  constexpr char kWriteRejected[] = "{\"command\":\"setAirconStat\",\"result\":2}";

  {  // Accepted is not confirmed until an authoritative matching readback arrives.
    wfrac_device_model::DeviceState state{};
    ScriptedTransport transport({{{}, kWriteAccepted}, {{}, read_on.c_str()}}, &state);
    FakeTimeProvider time{};
    Fixture fixture{};
    wfrac_client::PostWriteVerificationSession session{};
    wfrac_client::ControlVerificationReport report{};
    assert(begin(on_frame, &time, &transport, &state, &session, &report, &fixture, 1U) ==
           ControlVerificationOutcome::accepted);
    assert(state.pending.phase == wfrac_device_model::PendingCommandPhase::verifying &&
           state.desired.power == wfrac_device_model::PowerState::on);
    assert(readback(&time, &transport, &state, &session, &report, &fixture) == ControlVerificationOutcome::confirmed);
    assert(state.observed.has_value && state.observed.value.power == wfrac_device_model::PowerState::on &&
           !state.pending.intent.has_value() && !state.desired.power.has_value() && transport.calls() == 2U);
  }

  {  // A mismatching authoritative observation is retained and never overwritten by desired state.
    wfrac_device_model::DeviceState state{};
    ScriptedTransport transport({{{}, kWriteAccepted}, {{}, read_off.c_str()}, {{}, read_on.c_str()}}, &state);
    FakeTimeProvider time{};
    Fixture fixture{};
    wfrac_client::PostWriteVerificationSession session{};
    wfrac_client::ControlVerificationReport report{};
    assert(begin(on_frame, &time, &transport, &state, &session, &report, &fixture, 1U) ==
           ControlVerificationOutcome::accepted);
    assert(readback(&time, &transport, &state, &session, &report, &fixture) ==
           ControlVerificationOutcome::not_confirmed);
    assert(session.conflicting_observation_seen && state.observed.has_value &&
           state.observed.value.power == wfrac_device_model::PowerState::off &&
           state.desired.power == wfrac_device_model::PowerState::on && !state.pending.intent.has_value() &&
           transport.calls() == 2U);
  }

  {  // A bounded second caller-scheduled readback may observe eventual convergence.
    wfrac_device_model::DeviceState state{};
    ScriptedTransport transport({{{}, kWriteAccepted}, {{}, read_off.c_str()}, {{}, read_on.c_str()}}, &state);
    FakeTimeProvider time{};
    Fixture fixture{};
    wfrac_client::PostWriteVerificationSession session{};
    wfrac_client::ControlVerificationReport report{};
    assert(begin(on_frame, &time, &transport, &state, &session, &report, &fixture, 2U) ==
           ControlVerificationOutcome::accepted);
    assert(readback(&time, &transport, &state, &session, &report, &fixture) == ControlVerificationOutcome::verifying);
    assert(state.pending.intent.has_value() && state.observed.value.power == wfrac_device_model::PowerState::off);
    assert(readback(&time, &transport, &state, &session, &report, &fixture) == ControlVerificationOutcome::confirmed);
    assert(!state.pending.intent.has_value() && transport.calls() == 3U);
  }

  {  // Readback timeout is terminal and does not retry the write.
    wfrac_device_model::DeviceState state{};
    ScriptedTransport transport({{{}, kWriteAccepted}, {wfrac_transport::TransportError::timeout, nullptr}}, &state);
    FakeTimeProvider time{};
    Fixture fixture{};
    wfrac_client::PostWriteVerificationSession session{};
    wfrac_client::ControlVerificationReport report{};
    assert(begin(on_frame, &time, &transport, &state, &session, &report, &fixture, 2U) ==
           ControlVerificationOutcome::accepted);
    assert(readback(&time, &transport, &state, &session, &report, &fixture) ==
           ControlVerificationOutcome::readback_transport_failure);
    assert(report.transport_error == wfrac_transport::TransportError::timeout && !state.pending.intent.has_value() &&
           state.desired.power == wfrac_device_model::PowerState::on && transport.calls() == 2U);
  }

  {  // Write transport and application failures never enter verification.
    wfrac_device_model::DeviceState transport_state{};
    ScriptedTransport transport_failure({{wfrac_transport::TransportError::connection_failure, nullptr}}, &transport_state);
    FakeTimeProvider transport_time{};
    Fixture transport_fixture{};
    wfrac_client::PostWriteVerificationSession transport_session{};
    wfrac_client::ControlVerificationReport transport_report{};
    assert(begin(on_frame, &transport_time, &transport_failure, &transport_state, &transport_session, &transport_report,
                 &transport_fixture, 1U) == ControlVerificationOutcome::write_transport_failure);
    assert(!transport_state.pending.intent.has_value() && transport_failure.calls() == 1U);

    wfrac_device_model::DeviceState rejected_state{};
    ScriptedTransport rejected({{{}, kWriteRejected}}, &rejected_state);
    FakeTimeProvider rejected_time{};
    Fixture rejected_fixture{};
    wfrac_client::PostWriteVerificationSession rejected_session{};
    wfrac_client::ControlVerificationReport rejected_report{};
    assert(begin(on_frame, &rejected_time, &rejected, &rejected_state, &rejected_session, &rejected_report,
                 &rejected_fixture, 1U) == ControlVerificationOutcome::write_rejected);
    assert(rejected_report.application_result == 2 && !rejected_state.pending.intent.has_value() && rejected.calls() == 1U);
  }

  return 0;
}
