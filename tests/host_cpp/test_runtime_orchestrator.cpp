#include <array>
#include <cassert>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "wfrac_client/runtime_orchestrator.hpp"
#include "wfrac_protocol/base64.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {
using namespace wfrac_client;
namespace dm = wfrac_device_model;
using TE = wfrac_transport::TransportError;

struct Clock final : IMonotonicClock {
  uint64_t value = 0;
  uint64_t now_ms() override { return value; }
};
struct WallTime final : wfrac_api::ITimeProvider {
  wfrac_api::TimeSample sample{5000000000LL, wfrac_api::TimeQuality::estimated};
  std::size_t calls = 0;
  wfrac_api::TimeSample now() override { ++calls; return sample; }
};

std::string observation(bool on, bool corrupt = false, bool segmented = false) {
  std::vector<uint8_t> raw(segmented ? 46U : 42U, 0);
  auto crc = wfrac_protocol::crc16_ccitt(raw.data(), 19U);
  raw[19] = static_cast<uint8_t>(crc);
  raw[20] = static_cast<uint8_t>(crc >> 8);
  raw[23] = on ? 0x09U : 0x08U;
  raw[24] = 2;
  raw[25] = 47;
  raw[26] = 154;
  if (segmented) { raw[39] = 1; raw[40] = 0xFF; raw[41] = 0xFF; raw[42] = 0xFF; raw[43] = 0xFF; }
  const auto end = raw.size() - 2;
  crc = wfrac_protocol::crc16_ccitt(raw.data() + 21, end - 21);
  raw[end] = static_cast<uint8_t>(crc) ^ (corrupt ? 1U : 0U);
  raw[end + 1] = static_cast<uint8_t>(crc >> 8);
  std::array<uint8_t, 128> encoded{};
  std::size_t size = 0;
  assert(wfrac_protocol::base64_encode({raw.data(), raw.size()}, {encoded.data(), encoded.size()}, &size) ==
         wfrac_protocol::Base64Error::none);
  return "{\"command\":\"getAirconStat\",\"result\":0,\"contents\":{\"airconStat\":\"" +
      std::string(reinterpret_cast<char*>(encoded.data()), size) + "\"}}";
}
const char* accepted = "{\"command\":\"setAirconStat\",\"result\":0}";
struct Reply { std::string path; TE error; std::string body; };
struct Transport final : wfrac_transport::ITransport {
  std::vector<Reply> script;
  std::vector<wfrac_transport::TransportScheme> schemes;
  std::vector<std::string> bodies;
  std::function<void()> hook;
  std::size_t calls = 0;
  TE perform(const wfrac_transport::TransportRequest& request, wfrac_transport::MutableByteView out,
             wfrac_transport::TransportResponse* response) override {
    assert(calls < script.size());
    const auto& reply = script[calls++];
    assert(reply.path == request.path);
    assert(request.timeout_ms == 1000U);
    schemes.push_back(request.endpoint.scheme);
    bodies.emplace_back(reinterpret_cast<const char*>(request.body.data), request.body.size);
    if (hook) hook();
    *response = {};
    if (reply.error != TE::none) return reply.error;
    assert(reply.body.size() <= out.size);
    std::memcpy(out.data, reply.body.data(), reply.body.size());
    response->body_size = reply.body.size();
    response->http_status = 200;
    return TE::none;
  }
  void read(std::string body, TE error = TE::none) {
    script.push_back({"/beaver/command/getAirconStat", error, std::move(body)});
  }
  void write(TE error = TE::none) { script.push_back({"/beaver/command/setAirconStat", error, accepted}); }
};
struct Fixture {
  Clock clock;
  WallTime wall;
  Transport transport;
  dm::DeviceState state{};
  wfrac_transport::SchemeCache cache{};
  std::array<uint8_t, 1024> request{}, response{};
  std::array<uint8_t, 128> scratch{}, raw{}, control{};
  RuntimeOrchestrator runtime;
  RuntimePolicy policy{100, 10, 20, 15, 80, 60, 2, 2, 2, 2};  // Synthetic test policy only.
  RuntimeBindings bindings() {
    return {{"1.0", "synthetic-device", "synthetic-operator"}, "synthetic-aircon", &wall, &clock,
        {"example.invalid", 51443, 1000}, &cache, &transport, &state,
        {{request.data(), request.size()}, {response.data(), response.size()},
         {scratch.data(), scratch.size()}, {raw.data(), raw.size()}}, {control.data(), control.size()}};
  }
  void init() { assert(runtime.initialize(bindings(), policy) == RuntimeError::none); }
  void baseline(bool on = false) {
    transport.read(observation(on));
    assert(runtime.step().outcome == RuntimeOutcome::observation);
    assert(state.availability == dm::AvailabilityState::available);
  }
  void power(bool on) {
    assert(runtime.enqueue(dm::SetPowerIntent{on ? dm::PowerState::on : dm::PowerState::off}) == RuntimeError::none);
  }
};

void validation_and_queue() {
  Fixture f;
  assert(f.runtime.step().error == RuntimeError::invalid_argument);
  auto b = f.bindings();
  b.clock = nullptr;
  assert(f.runtime.initialize(b, f.policy) == RuntimeError::invalid_argument);
  b = f.bindings(); b.control_raw = b.buffers.raw_aircon_stat;
  assert(f.runtime.initialize(b, f.policy) == RuntimeError::invalid_argument);
  auto p = f.policy; p.poll_interval_ms = 0;
  assert(f.runtime.initialize(f.bindings(), p) == RuntimeError::invalid_argument);
  p = f.policy; p.queue_capacity = 5;
  assert(f.runtime.initialize(f.bindings(), p) == RuntimeError::invalid_argument);
  f.init();
  assert(f.runtime.initialize(f.bindings(), f.policy) == RuntimeError::busy);
  assert(f.runtime.enqueue(dm::SetPowerIntent{dm::PowerState::unknown}) == RuntimeError::invalid_argument);
  assert(f.runtime.enqueue(dm::SetTargetTemperatureIntent{2323}) == RuntimeError::invalid_argument);
  f.power(true);
  assert(f.runtime.enqueue(dm::SetModeIntent{dm::HvacMode::heat}) == RuntimeError::none);
  assert(f.runtime.enqueue(dm::SetFanIntent{dm::FanSetting::speed3}) == RuntimeError::queue_full);
  assert(!f.state.desired.fan.has_value());
  f.power(false);  // Coalesce queued field without growing the bounded queue.
  assert(f.runtime.queued_commands() == 2 && f.state.desired.power == dm::PowerState::off);
  f.baseline(false);
  assert(f.runtime.step().action == RuntimeAction::discard_satisfied);
  assert(f.runtime.queued_commands() == 1 && f.transport.calls == 1);
}

void serialization_and_superseding() {
  Fixture f; f.init(); f.baseline(); f.power(true);
  f.transport.write();
  assert(f.runtime.step().outcome == RuntimeOutcome::accepted);
  assert(f.state.observed.value.power == dm::PowerState::off);
  f.power(false);  // New desired must survive older pending convergence.
  assert(f.runtime.step().action == RuntimeAction::idle);
  f.clock.value = 20;
  f.transport.read(observation(true, false, true));  // Different frame size becomes next baseline.
  auto r = f.runtime.step();
  assert(r.outcome == RuntimeOutcome::confirmed && r.verification.validated_raw_aircon_stat_size == 46);
  assert(f.state.desired.power == dm::PowerState::off && !f.runtime.verifying());
  f.transport.write();
  assert(f.runtime.step().outcome == RuntimeOutcome::accepted);
  assert(std::get<dm::SetPowerIntent>(*f.state.pending.intent).value == dm::PowerState::off);
  f.clock.value = 40;
  f.transport.read(observation(false));
  assert(f.runtime.step().outcome == RuntimeOutcome::confirmed);
  assert(!f.state.desired.power.has_value() && f.transport.calls == 5);
  // These tests establish module observation convergence, never physical actuation.
}

void verification_and_deadlines() {
  Fixture f; f.init(); f.baseline(); f.power(true); f.transport.write();
  assert(f.runtime.step().outcome == RuntimeOutcome::accepted);
  f.clock.value = 20; f.transport.read(observation(false));
  assert(f.runtime.step().outcome == RuntimeOutcome::verifying);
  f.clock.value = 34;
  assert(f.runtime.step().action == RuntimeAction::idle);
  f.clock.value = 35; f.transport.read(observation(false));
  assert(f.runtime.step().outcome == RuntimeOutcome::not_confirmed);
  assert(!f.state.pending.intent.has_value() && f.state.desired.power == dm::PowerState::on);
  assert(f.runtime.step().action == RuntimeAction::idle); // Never auto-resend desired.

  Fixture expired; expired.init(); expired.baseline(); expired.power(true); expired.transport.write();
  assert(expired.runtime.step().outcome == RuntimeOutcome::accepted);
  expired.clock.value = 80;
  assert(expired.runtime.step().action == RuntimeAction::expire_command);
  assert(!expired.state.pending.intent.has_value() && expired.transport.calls == 2);
  Fixture elapsed; elapsed.init(); elapsed.baseline(); elapsed.power(true); elapsed.transport.write();
  elapsed.transport.hook = [&] { elapsed.clock.value = 80; };
  assert(elapsed.runtime.step().outcome == RuntimeOutcome::timed_out);
  assert(!elapsed.runtime.verifying() && !elapsed.state.pending.intent.has_value());
  Fixture late; late.init(); late.baseline(); late.power(true); late.transport.write();
  assert(late.runtime.step().outcome == RuntimeOutcome::accepted);
  late.clock.value = 20; late.transport.read(observation(true));
  late.transport.hook = [&] { late.clock.value = 80; };
  assert(late.runtime.step().outcome == RuntimeOutcome::timed_out);
  assert(late.state.observed.value.power == dm::PowerState::on && !late.runtime.verifying());
}

void retry_availability_and_safety() {
  Fixture f; f.init(); f.transport.read("", TE::timeout);
  assert(f.runtime.step().poll_retry_scheduled);
  assert(f.state.availability == dm::AvailabilityState::unknown);
  f.clock.value = 9; assert(f.runtime.step().action == RuntimeAction::idle);
  f.clock.value = 10; f.transport.read("", TE::connection_failure);
  assert(!f.runtime.step().poll_retry_scheduled);
  assert(f.state.availability == dm::AvailabilityState::unavailable);
  f.clock.value = 109; assert(f.runtime.step().action == RuntimeAction::idle);
  f.clock.value = 110; f.baseline();
  assert(f.transport.calls == 3);

  for (auto error : {TE::tls_failure, TE::tls_identity_failure, TE::response_too_large, TE::invalid_argument}) {
    Fixture tls; tls.init(); tls.transport.read("", error);
    assert(!tls.runtime.step().poll_retry_scheduled);
    assert(tls.transport.calls == 1 && tls.transport.schemes[0] == wfrac_transport::TransportScheme::https);
  }
  Fixture write; write.init(); write.baseline(); write.power(true); write.transport.write(TE::timeout);
  assert(write.runtime.step().outcome == RuntimeOutcome::failed);
  assert(write.runtime.queued_commands() == 0 && !write.state.pending.intent.has_value());
  assert(write.runtime.step().action == RuntimeAction::idle && write.transport.calls == 2);
  assert(write.state.availability == dm::AvailabilityState::available); // Write alone not read evidence.

  Fixture read; read.init(); read.baseline(); read.power(true); read.transport.write();
  assert(read.runtime.step().outcome == RuntimeOutcome::accepted);
  read.clock.value = 20; read.transport.read("", TE::timeout);
  assert(read.runtime.step().outcome == RuntimeOutcome::failed);
  assert(!read.runtime.verifying() && !read.state.pending.intent.has_value());
  read.clock.value = 35; assert(read.runtime.step().action == RuntimeAction::idle);

  Fixture bad; bad.init(); bad.transport.read(observation(true, true));
  assert(bad.runtime.step().response_error == wfrac_api::ApiResponseError::crc_mismatch);
  assert(!bad.state.observed.has_value);

  Fixture fallback; fallback.init();
  fallback.transport.read("", TE::scheme_mismatch);
  fallback.transport.read(observation(false));
  assert(fallback.runtime.step().outcome == RuntimeOutcome::observation);
  assert(fallback.transport.calls == 2 && fallback.wall.calls == 1);
  assert(fallback.transport.bodies[0] == fallback.transport.bodies[1]);
  assert(fallback.transport.schemes[0] == wfrac_transport::TransportScheme::https &&
         fallback.transport.schemes[1] == wfrac_transport::TransportScheme::http);
  fallback.power(true); fallback.transport.write();
  assert(fallback.runtime.step().outcome == RuntimeOutcome::accepted);
  assert(fallback.transport.schemes[2] == wfrac_transport::TransportScheme::http);
}

void time_baseline_and_reentrancy() {
  Fixture f; f.init(); f.wall.sample = {};
  assert(f.runtime.step().request_error == wfrac_api::ApiRequestError::time_unavailable);
  assert(f.transport.calls == 0 && f.state.availability == dm::AvailabilityState::unknown);
  f.wall.sample = {5000000000LL, wfrac_api::TimeQuality::trusted};
  f.clock.value = 100; f.baseline();
  assert(f.transport.bodies[0].find("5000000000") != std::string::npos);
  f.power(true); f.clock.value = 161;
  assert(f.runtime.step().action == RuntimeAction::idle); // Stale baseline waits for due poll.
  f.clock.value = 200; f.transport.read(observation(false));
  assert(f.runtime.step().action == RuntimeAction::poll);
  f.transport.write();
  f.transport.hook = [&] {
    assert(f.runtime.step().error == RuntimeError::busy);
    assert(f.runtime.enqueue(dm::SetPowerIntent{dm::PowerState::off}) == RuntimeError::busy);
  };
  assert(f.runtime.step().outcome == RuntimeOutcome::accepted);
  f.clock.value = 199;
  assert(f.runtime.step().error == RuntimeError::clock_regressed);
  f.clock.value = 300;
  assert(f.runtime.step().error == RuntimeError::clock_regressed); // Latched, no invented recovery.

  Fixture max; max.clock.value = std::numeric_limits<uint64_t>::max() - 5;
  max.init(); max.baseline(); max.power(true); max.transport.write();
  assert(max.runtime.step().outcome == RuntimeOutcome::accepted);
  max.clock.value = std::numeric_limits<uint64_t>::max();
  assert(max.runtime.step().outcome == RuntimeOutcome::timed_out); // No deadline wrap.
}
}  // namespace

int main() {
  validation_and_queue();
  serialization_and_superseding();
  verification_and_deadlines();
  retry_availability_and_safety();
  time_baseline_and_reentrancy();
}
