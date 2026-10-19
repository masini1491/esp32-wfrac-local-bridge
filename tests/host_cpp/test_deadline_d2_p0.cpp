// D2-P0-T TEST-ONLY: scripted controllable I/O and synthetic private ownership.
// Reuse accepted D1's exact parser/policy fixtures and all 36 baseline cases.
// NOT an ITransport implementation, SDK socket/TLS execution, or hardware proof.
#define WFRAC_D1_FIXTURES_ONLY
#include "test_deadline_d1.cpp"
#undef WFRAC_D1_FIXTURES_ONLY

namespace { // Reusable test fixtures have TU-local linkage, like their D1 types.
namespace d2 {
enum class Op { dns, connect_dispatch, ready, handshake, send, receive };
enum class Hint { ok, interrupted, again, want_read, want_write, failure };
struct Step {
  Op op;
  std::int64_t elapsed = 0;
  Hint hint = Hint::ok;
  std::string bytes{};
  std::size_t accepted = 0;
};
struct Script {
  std::vector<Step> steps;
  std::vector<std::int64_t> waits;
  std::vector<std::string> offered;
  std::size_t next = 0;
  std::size_t delivered = 0;
  Step take(Op op, std::int64_t remaining, const std::string& bytes) {
    require(remaining > 0, "never pass zero/infinite wait to fake backend");
    require(steps.size() <= 64 && next < steps.size(), "bounded scripted backend exhausted");
    require(steps[next].op == op, "unexpected fake I/O dispatch");
    waits.push_back(remaining); offered.push_back(bytes);
    if (op == Op::send && steps[next].hint == Hint::ok) delivered += steps[next].accepted;
    return steps[next++];
  }
};
enum class Close { acknowledged, error, unknown };
struct Token { unsigned generation = 1; };
struct PrivateOwner {
  Life state = Life::active;
  unsigned generation = 1, in_flight = 0, callback_finishes = 0;
  unsigned close_calls = 0, frees = 0, rejected = 0;
  bool pending_token = false, fence_seen = false, quarantined = false;
  bool close_attempted = false, close_ack = false, unknown_fd = false;
  bool socket_owned = true, tls_owned = true, bio_owned = true, parser_owned = true;
  bool protocol_waitstate = false; // Synthetic protocol residue != application refs.
  std::array<unsigned char, 8> private_bytes{}; // No caller buffer pointer in token/owner.
  std::vector<Life> transitions{Life::active};
  bool begin(Token token) {
    if (token.generation != generation || state != Life::active || !pending_token || in_flight) {
      ++rejected; return false;
    }
    ++in_flight; private_bytes[0] = 42; return true;
  }
  bool end(Token token) {
    if (token.generation != generation || in_flight != 1) return false;
    --in_flight; ++callback_finishes; return true;
  }
  bool fence(bool executed = true) {
    if (!executed || !pending_token || in_flight || callback_finishes != 1) return false;
    fence_seen = true; pending_token = false; return true;
  }
  void terminal() {
    require(state == Life::active, "one terminal decision");
    state = Life::terminal_decided; transitions.push_back(state);
  }
  bool stop(Close witness) {
    require(state == Life::terminal_decided || state == Life::stopping, "stop lifecycle");
    if (state != Life::stopping) { state = Life::stopping; transitions.push_back(state); }
    if (pending_token || in_flight || unknown_fd) { quarantined = true; return false; }
    if (!close_attempted) {
      close_attempted = true; ++close_calls;
      close_ack = witness == Close::acknowledged;
      unknown_fd = !close_ack;
    }
    if (!close_ack) { quarantined = true; return false; }
    quarantined = false; state = Life::quiescent; transitions.push_back(state); return true;
  }
  bool release() {
    if (state != Life::quiescent || pending_token || in_flight || !close_ack) return false;
    socket_owned = tls_owned = bio_owned = parser_owned = false;
    ++frees; state = Life::released; transitions.push_back(state); return true;
  }
  bool next_allowed() const { return state == Life::released; }
};
struct Identity {
  Scheme scheme = Scheme::https;
  std::string logical = "unit.invalid", http_host = "unit.invalid", sni = "unit.invalid";
  std::string address = "192.0.2.1"; // Documentation-only fake address; never connected.
  bool ca_required = true, chain_ok = true, name_ok = true, sni_ok = true;
};
struct OwnerSlot {
  PrivateOwner owner;
  bool occupied = false;
  unsigned attempts = 0;
  bool start() {
    if ((occupied && !owner.next_allowed()) || attempts == 2) return false;
    if (occupied) {
      const unsigned generation = owner.generation + 1;
      owner = PrivateOwner{}; owner.generation = generation;
    }
    occupied = true; ++attempts; return true;
  }
};
struct Probe {
  Clock clock;
  Budget budget;
  Framing framing;
  Script script;
  PrivateOwner owner;
  Identity identity;
  Response& out; // Synchronous publication only; callbacks receive PrivateOwner, not Probe.
  Error error = Error::none;
  unsigned connects = 0, posts = 0;
  std::size_t sent = 0;
  bool terminal = false, connected = false, identity_valid = false, established = false;
  std::int64_t io_end = -1;
  explicit Probe(Response& response, std::vector<Step> steps = {},
                 std::uint32_t timeout = 10, std::size_t capacity = 32,
                 std::int64_t start = 0)
      : clock{start}, budget(clock, timeout), framing(capacity),
        script{std::move(steps), {}, {}, 0, 0}, out(response) { out = {}; }
  void decide(Error result) {
    if (terminal) return;
    error = result; terminal = true; io_end = clock.now; out = {}; owner.terminal();
  }
  bool checkpoint() {
    if (terminal) return false;
    if (budget.remaining() == 0) { decide(budget.error); return false; }
    return true;
  }
  bool verify_identity() {
    if (!checkpoint()) return false;
    if (identity.http_host != identity.logical) { decide(Error::invalid_argument); return false; }
    if (identity.scheme == Scheme::https &&
        (!identity.ca_required || !identity.chain_ok || !identity.name_ok || !identity.sni_ok ||
         identity.sni != identity.logical)) { decide(Error::tls_identity_failure); return false; }
    identity_valid = true; return true;
  }
  bool io(Op op) {
    if (!checkpoint()) return false;
    if (op == Op::connect_dispatch && connects != 0) { decide(Error::io_failure); return false; }
    const std::string write = op == Op::send ? std::string("POST").substr(sent) : std::string{};
    if (op == Op::send) {
      if (sent == 4) { decide(Error::io_failure); return false; }
      if (posts == 0) ++posts; // Short writes and TLS retries share one logical POST.
    }
    const Step step = script.take(op, budget.remaining(), write);
    if (op == Op::connect_dispatch) ++connects;
    // Native dispatch may be unpreemptible. This fake advances then checks;
    // a late return is observable, never mislabeled a preemptive caller bound.
    if (!clock.advance(step.elapsed)) { decide(Error::io_failure); return false; }
    if (!checkpoint()) return false;
    if (step.hint == Hint::failure) {
      decide(op == Op::connect_dispatch ? Error::connection_failure :
             op == Op::handshake ? Error::tls_failure : Error::io_failure);
      return false;
    }
    if (step.hint != Hint::ok) {
      require(step.accepted == 0 && step.bytes.empty(), "retry hint cannot invent delivered bytes");
      return true;
    }
    if (op == Op::connect_dispatch) {
      connected = true;
      established = identity_valid && identity.scheme == Scheme::http;
    }
    if (op == Op::handshake && connected && identity_valid) established = true;
    if (op == Op::send) {
      require(step.accepted <= write.size(), "short write cannot exceed offered bytes");
      sent += step.accepted;
    }
    if (op == Op::receive) {
      const Error parsed = framing.feed(step.bytes);
      if (parsed != Error::none) { decide(parsed); return false; }
    }
    return true;
  }
  void finish(bool eof = true, std::int64_t cleanup_delay = 0,
              Close witness = Close::acknowledged) {
    if (!terminal && checkpoint()) {
      Error result = eof ? framing.eof() : framing.error;
      if (result == Error::none && !framing.final_complete) result = Error::io_failure;
      if (result == Error::none && !established) result = Error::io_failure;
      decide(result);
    }
    // response_too_large is positive scheme evidence in existing policy. Never
    // emit it for a synthetic channel lacking the applicable handshake witness.
    if (error == Error::response_too_large && !established) error = Error::io_failure;
    require(clock.advance(cleanup_delay), "synthetic cleanup clock overflow");
    if (!owner.stop(witness)) {
      if (error == Error::none) error = Error::io_failure;
      out = {}; return;
    }
    require(owner.release(), "success needs observed quiescence then release");
    if (error == Error::none) out = {framing.status, framing.body.size()};
    else if (error == Error::response_too_large && established) out = {framing.status, 0};
  }
};

void register_tests() {
  add("D2/A shared budget across DNS connect readiness TLS and short writes", [] {
    Response out{9,9}; Probe p(out, {{Op::dns,1}, {Op::connect_dispatch,1}, {Op::ready,1},
      {Op::handshake,1,Hint::want_read}, {Op::handshake,1,Hint::want_write},
      {Op::handshake,1}, {Op::send,1,Hint::ok,{},1}, {Op::send,1,Hint::ok,{},3},
      {Op::receive,1,Hint::ok,cl()}}, 10);
    require(p.verify_identity(), "fake TLS prerequisites");
    for (Op op : {Op::dns,Op::connect_dispatch,Op::ready,Op::handshake,Op::handshake,
                  Op::handshake,Op::send,Op::send,Op::receive}) require(p.io(op), "script step");
    require(out.body_size == 0 && p.script.waits == std::vector<std::int64_t>{10,9,8,7,6,5,4,3,2},
            "one shrinking budget, no early metadata");
    require(p.script.offered[6] == "POST" && p.script.offered[7] == "OST", "short write offset");
    p.finish(); require(p.error == Error::none && out.body_size == 3 && p.posts == 1 && p.connects == 1,
                        "one response after teardown");
  });
  add("D2/A delayed DNS completion cannot initiate connect", [] {
    Response out; Probe p(out, {{Op::dns,11},{Op::connect_dispatch,0}});
    require(!p.io(Op::dns), "late DNS"); p.io(Op::connect_dispatch); p.finish();
    require(p.error == Error::timeout && p.clock.now == 11 && p.script.next == 1 && !p.connects,
            "return post-check, no fictitious hard cap or post-timeout connect");
  });
  add("D2/A unpreemptible connect dispatch returns after budget", [] {
    Response out; Probe p(out, {{Op::connect_dispatch,17},{Op::send,0}});
    p.io(Op::connect_dispatch); p.io(Op::send); p.finish();
    require(p.error == Error::timeout && p.clock.now == 17 && p.connects == 1 && !p.posts,
            "late dispatch recognized before new I/O");
  });
  add("D2/A EINTR EAGAIN and TLS WANT hints never renew budget", [] {
    for (Hint hint : {Hint::interrupted, Hint::again, Hint::want_read, Hint::want_write}) {
      Response out; Probe p(out, {{Op::ready,4,hint},{Op::ready,1,hint},{Op::send,0}},5);
      require(p.io(Op::ready) && !p.io(Op::ready), "expiry on retry"); p.io(Op::send); p.finish();
      require(p.script.waits == std::vector<std::int64_t>{5,1} && p.script.next == 2 &&
              p.error == Error::timeout && !p.posts && out.body_size == 0, "no infinite/renewed wait");
    }
  });
  add("D2/A header and body trickle consume the original deadline", [] {
    for (bool body : {false,true}) {
      std::vector<Step> steps;
      if (body) steps.push_back({Op::receive,0,Hint::ok,"HTTP/1.1 200 OK\r\nContent-Length: 20\r\n\r\n"});
      for (int i=0;i<11;++i) steps.push_back({Op::receive,1,Hint::ok,body?"a":cl().substr(i,1)});
      Response out; Probe p(out, steps);
      while (!p.terminal) p.io(Op::receive);
      const auto called = p.script.next; p.io(Op::receive); p.finish();
      require(p.error == Error::timeout && p.script.next == called && out.body_size == 0, "trickle fail closed");
    }
  });
  add("D2/A zero overflow and regressing clock dispatch no extra I/O", [] {
    for (bool overflow : {false,true}) {
      Response out{9,9}; Probe p(out, {{Op::ready,0}}, overflow?3:0,32,
                                      overflow?std::numeric_limits<std::int64_t>::max()-2:0);
      p.io(Op::ready); p.finish();
      require(p.error == Error::invalid_argument && !p.script.next && out.body_size == 0, "invalid budget");
    }
    Response out; Probe p(out, {{Op::ready,1},{Op::ready,0}},10,32,5);
    p.io(Op::ready); p.clock.now = 4; p.io(Op::ready); p.finish();
    require(p.error == Error::io_failure && p.script.next == 1, "clock regression gates backend");
  });
  add("D2/A completed I/O and synchronous cleanup have separate clocks", [] {
    Response out; Probe p(out, {{Op::connect_dispatch,0},{Op::handshake,0},{Op::receive,2,Hint::ok,cl()}});
    p.verify_identity(); p.io(Op::connect_dispatch); p.io(Op::handshake); p.io(Op::receive); p.finish(true,25);
    require(p.error == Error::none && p.io_end == 2 && p.clock.now == 27 && out.body_size == 3,
            "safe cleanup overrun permitted, not hard return bound");
  });
  add("D2/A frozen fake clock is not a preemptive timeout proof", [] {
    Response out; Probe p(out, std::vector<Step>(64, {Op::ready,0,Hint::again}));
    for (int i=0;i<64;++i) require(p.io(Op::ready), "same-tick retries");
    bool rejected=false;
    try { p.io(Op::ready); } catch (const std::runtime_error&) { rejected=true; }
    require(rejected && p.clock.now == 0 && p.script.next == 64 && out.body_size == 0,
            "finite script exhaustion, not a fabricated native timeout");
    p.decide(Error::io_failure); p.finish(false);
  });
  add("D2/A timeout remains timeout after safe cleanup overrun", [] {
    Response out; Probe p(out, {{Op::send,10,Hint::ok,{},1}});
    p.io(Op::send); p.finish(false,25);
    require(p.error == Error::timeout && p.io_end == 10 && p.clock.now == 35 &&
            p.owner.state == Life::released && out.body_size == 0, "timeout not cleanup failure");
  });
  add("D2/B partial malformed truncated interim and bad chunks stay private", [] {
    for (const auto& wire : {std::string("HTTP/1.1 200 OK\r\nContent-Len"),
         std::string("not-http"),std::string("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nabc"),
         std::string("HTTP/1.1 100 Continue\r\n\r\n"),
         std::string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\n")}) {
      Response out{9,9}; Probe p(out, {{Op::receive,1,Hint::ok,wire}});
      p.io(Op::receive); require(out.body_size == 0 && out.http_status == 0, "no parser-stage publish");
      p.finish(); require(p.error == Error::io_failure && out.body_size == 0 && out.http_status == 0,
                          "bad response cannot publish");
    }
  });
  add("D2/B final non-2xx chunked response publishes only after teardown", [] {
    Response out{9,9}; Probe p(out, {{Op::connect_dispatch,0},{Op::handshake,0},{Op::receive,1,Hint::ok,
      "HTTP/1.1 103 Early Hints\r\n\r\nHTTP/1.1 429 Busy\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n"}});
    p.verify_identity(); p.io(Op::connect_dispatch); p.io(Op::handshake); p.io(Op::receive);
    require(p.framing.final_complete && out.body_size == 0, "complete bytes private");
    p.finish(); require(p.error == Error::none && out.http_status == 429 && out.body_size == 3,
                        "HTTP status is application data");
  });
  add("D2/B overflow scheme evidence requires an established transport", [] {
    for (bool established : {false,true}) {
      Response out; Probe p(out, established?std::vector<Step>{{Op::connect_dispatch,0},{Op::handshake,0},{Op::receive,1,Hint::ok,cl()}}:
                            std::vector<Step>{{Op::connect_dispatch,0},{Op::receive,1,Hint::ok,cl()}},10,2);
      p.verify_identity(); p.io(Op::connect_dispatch);
      if (established) p.io(Op::handshake);
      p.io(Op::receive); p.finish();
      require(p.error == (established?Error::response_too_large:Error::io_failure) && out.body_size == 0 &&
              out.http_status == (established?200:0), "no fabricated established scheme");
      wfrac_transport::SchemeCache cache;
      const auto decision = wfrac_transport::observe_scheme_attempt(&cache,Scheme::https,p.error);
      if (established) {
        require(decision.next_action ==
                wfrac_transport::SchemeNextAction::stop &&
                cache.knowledge == wfrac_transport::SchemeKnowledge::https_confirmed, "existing overflow policy");
      } else {
        require(cache.knowledge == wfrac_transport::SchemeKnowledge::unknown, "TCP alone is not HTTPS evidence");
      }
    }
  });
  add("D2/B maybe-delivered write is never retried or downgraded", [] {
    Response out; Probe p(out, {{Op::connect_dispatch,0},{Op::send,11,Hint::ok,{},2},{Op::send,0}});
    p.io(Op::connect_dispatch); p.io(Op::send); p.io(Op::send); p.io(Op::connect_dispatch); p.finish();
    require(p.error == Error::timeout && p.script.next == 2 && p.posts == 1 && p.connects == 1 &&
            p.script.delivered == 2 && p.identity.scheme == Scheme::https && out.body_size == 0,
            "fake peer may have bytes despite timeout; no retry on delivery uncertainty");
  });
  add("D2/B TLS short-write retries preserve exact unsent bytes", [] {
    Response out; Probe p(out, {{Op::send,1,Hint::want_write},{Op::send,1,Hint::want_read},
      {Op::send,1,Hint::ok,{},1},{Op::send,1,Hint::again},{Op::send,1,Hint::ok,{},3}});
    for (int i=0;i<5;++i) require(p.io(Op::send), "short write step");
    require(p.script.offered == std::vector<std::string>{"POST","POST","POST","OST","OST"} &&
            p.sent == 4 && p.posts == 1, "TLS retry != another HTTP attempt");
    p.io(Op::send); p.finish(false);
    require(p.error == Error::io_failure && p.script.next == 5, "second POST forbidden");
  });
  add("D2/B primary connection and TLS errors survive uncertain cleanup", [] {
    for (Op op : {Op::connect_dispatch,Op::handshake}) {
      Response out; Probe p(out, {{op,1,Hint::failure}}); p.io(op); p.finish(false,0,Close::error);
      require(p.error == (op==Op::handshake?Error::tls_failure:Error::connection_failure) &&
              out.body_size == 0 && !p.owner.next_allowed(), "no cleanup masking or scheme mismatch");
    }
  });
  add("D2/C complete parser does not authorize success after failed close", [] {
    for (Close witness : {Close::error,Close::unknown}) {
      Response out{9,9}; Probe p(out, {{Op::connect_dispatch,0},{Op::handshake,0},{Op::receive,1,Hint::ok,cl()}});
      p.verify_identity(); p.io(Op::connect_dispatch); p.io(Op::handshake); p.io(Op::receive);
      p.finish(true,0,witness);
      require(p.error == Error::io_failure && out.body_size == 0 && out.http_status == 0 &&
              p.owner.quarantined && p.owner.unknown_fd && !p.owner.release() &&
              !p.owner.next_allowed(), "untrusted cleanup cannot commit response");
      require(!p.owner.stop(Close::acknowledged) && p.owner.close_calls == 1, "no blind second close");
    }
  });
  add("D2/C in-flight DNS owner and delayed fence prevent release", [] {
    Response out; Probe p(out); Token token; p.owner.pending_token = true;
    require(p.owner.begin(token), "callback admitted into private owner");
    p.decide(Error::timeout); p.finish(false);
    require(p.owner.state == Life::stopping && !p.owner.release() && !p.owner.next_allowed() &&
            p.owner.close_calls == 0 && !p.owner.fence(), "in-flight retained");
    require(p.owner.end(token) && !p.owner.fence(false) && !p.owner.stop(Close::acknowledged),
            "callback return/fence admission is not executed barrier");
    require(p.owner.fence() && p.owner.stop(Close::acknowledged) && p.owner.release() &&
            out.body_size == 0 && p.error == Error::timeout, "observed synthetic barrier permits release only");
  });
  add("D2/C absent callback consumes finite owner and blocks conflicting attempts", [] {
    OwnerSlot slot; require(slot.start(), "one owner admitted");
    auto& owner=slot.owner; owner.pending_token = true; owner.terminal();
    for (int i=0;i<4;++i) require(!owner.stop(Close::acknowledged) && !owner.release() &&
      !owner.fence() && !slot.start() && owner.pending_token, "actual admission gate blocks conflicting attempt");
    require(slot.attempts == 1 && owner.close_calls == 0 && owner.frees == 0, "finite slot, no new owner per retry");
  });
  add("D2/C late duplicate stale callbacks never write caller memory", [] {
    PrivateOwner owner; owner.pending_token = true; Token token;
    std::array<unsigned char,8> caller; caller.fill(77); const auto sentinel=caller;
    require(owner.begin(token) && !owner.begin(token) && owner.end(token), "one in-flight callback");
    owner.terminal(); const auto bytes=owner.private_bytes;
    require(!owner.begin(token) && !owner.begin({2}), "terminal/stale callback rejected");
    require(owner.fence() && owner.stop(Close::acknowledged) && owner.release(), "safe release");
    require(!owner.begin(token) && caller == sentinel && owner.private_bytes == bytes,
            "callback owns no caller pointer; no post-terminal publish");
  });
  add("D2/C bounded slot reuse needs executed fence and rejects old generation", [] {
    OwnerSlot slot; require(slot.start(), "first slot admission"); Token old;
    slot.owner.pending_token=true; require(slot.owner.begin(old) && slot.owner.end(old), "callback returns");
    slot.owner.terminal(); require(!slot.start() && !slot.owner.fence(false), "no reuse on queued fence");
    require(slot.owner.fence() && slot.owner.stop(Close::acknowledged) && slot.owner.release() &&
            slot.start() && slot.owner.generation == 2, "synthetic observed barrier permits next generation");
    slot.owner.pending_token=true;
    require(!slot.owner.begin(old) && slot.owner.begin({2}) && slot.owner.end({2}), "stale token cannot target new attempt");
    slot.owner.terminal(); require(slot.owner.fence() && slot.owner.stop(Close::acknowledged) &&
      slot.owner.release() && !slot.start(), "explicit finite two-generation fixture cap");
  });
  add("D2/C explicit lifecycle frees once and protocol residue is distinct", [] {
    PrivateOwner owner; owner.protocol_waitstate = true; owner.terminal();
    require(!owner.release() && owner.stop(Close::acknowledged) && owner.release() &&
            !owner.release() && owner.frees == 1 && owner.close_calls == 1 &&
            !owner.socket_owned && !owner.tls_owned && !owner.bio_owned && !owner.parser_owned &&
            owner.next_allowed() && owner.protocol_waitstate, "application refs vs synthetic protocol waitstate");
    require(owner.transitions == std::vector<Life>{Life::active,Life::terminal_decided,Life::stopping,
              Life::quiescent,Life::released}, "owner lifecycle order");
  });
  add("D2/C fixture termination is not a cleanup witness", [] {
    PrivateOwner owner; owner.pending_token=true; owner.terminal();
    const bool fixture_watchdog_fired=true; // Synthetic input, not an actual native hang.
    require(fixture_watchdog_fired && !owner.stop(Close::unknown) && !owner.release() &&
            owner.pending_token && owner.frees == 0, "TEST_TERMINATION does not prove quiescence");
  });
  add("D2/D HTTP HTTPS preserve hostname and logical TLS identity", [] {
    for (Scheme scheme : {Scheme::http,Scheme::https}) {
      Response out; Probe p(out, scheme==Scheme::https?
        std::vector<Step>{{Op::connect_dispatch,1},{Op::handshake,1},{Op::receive,1,Hint::ok,cl()}}:
        std::vector<Step>{{Op::connect_dispatch,1},{Op::receive,1,Hint::ok,cl()}});
      p.identity.scheme=scheme; require(p.verify_identity(), "both schemes supported in fake");
      p.io(Op::connect_dispatch); if (scheme==Scheme::https) p.io(Op::handshake);
      p.io(Op::receive); p.finish();
      require(p.identity.http_host == "unit.invalid" && p.identity.logical == "unit.invalid" &&
              p.identity.sni == "unit.invalid" && p.error == Error::none && out.body_size == 3,
              "address must not replace approved identity");
    }
  });
  add("D2/D chain hostname SNI and verify-required negative guards", [] {
    for (int failure=0;failure<5;++failure) {
      Response out{9,9}; Probe p(out, {{Op::connect_dispatch,0}});
      if (failure==0) p.identity.chain_ok=false;
      if (failure==1) p.identity.name_ok=false;
      if (failure==2) p.identity.sni_ok=false;
      if (failure==3) p.identity.ca_required=false;
      if (failure==4) p.identity.sni=p.identity.address;
      require(!p.verify_identity(), "fake insecure/identity outcome rejected"); p.io(Op::connect_dispatch); p.finish();
      require(p.error == Error::tls_identity_failure && !p.script.next && out.body_size == 0 &&
              p.identity.scheme == Scheme::https, "no CA/name/SNI bypass or HTTP downgrade");
    }
  });
  add("D2/D HTTP Host cannot silently become selected numeric address", [] {
    Response out; Probe p(out); p.identity.http_host=p.identity.address;
    require(!p.verify_identity() && p.error == Error::invalid_argument, "preserve logical HTTP Host");
    p.finish(false); require(out.body_size == 0, "host mismatch private");
  });
  add("D2/D absent TLS identity witness cannot establish trusted channel", [] {
    Response out; Probe p(out, {{Op::connect_dispatch,0},{Op::handshake,0},{Op::receive,1,Hint::ok,cl()}});
    p.io(Op::connect_dispatch); p.io(Op::handshake); p.io(Op::receive); p.finish();
    require(!p.established && p.error == Error::io_failure && out.body_size == 0 && out.http_status == 0,
            "handshake hint alone does not stand in for CA/name/SNI evidence");
  });
}
} // namespace d2
} // namespace

#ifndef WFRAC_D2_FIXTURES_ONLY
int main() {
  std::cout << "D1 OFFLINE TEST-ONLY; actual upstream parser version=" << http_parser_version()
            << "; no SDK timing/TLS/network/cleanup guarantee\n";
  parser_tests(); contract_tests();
  const std::size_t baseline = tests.size();
  require(baseline == 36, "retained D1 inventory drift needs explicit review");
  d2::register_tests();
  unsigned passed = 0;
  for (const auto& test : tests) {
    try { test.second(); ++passed; std::cout << "PASS " << test.first << '\n'; }
    catch (const std::exception& e) { std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; return 1; }
    if (passed == baseline) std::cout << "D1: 36/36 cases PASS (retained pinned parser + synthetic policy)\n";
  }
  std::cout << "D2-P0-T: " << passed-baseline << '/' << tests.size()-baseline
            << " SYNTHETIC_PASS; actual pinned http_parser reused, no SDK I/O/TLS/teardown runtime\n";
  std::cout << "NOT_RUN: native TLS/BIO/API compile/link, DNS/network/hardware, A6b/T1/A7b replay, C3/C6 matrix\n"
            << "UNPROVEN: hostname callback recovery, trusted native close witness, actual TLS verification; Gate A PENDING, Gate B HOLD\n";
}
#endif
