// D1 OFFLINE TEST-ONLY design harness. NOT an ESP-IDF adapter implementation.
// The actual pinned SDK parser is linked by run_deadline_d1.py, never vendored.
// Synthetic times/capacities are test inputs, not WF-RAC timing/size policy.
// No sockets, threads, TLS engine, DNS, hardware, or caller-return guarantee.
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "http_parser.h"
#include "wfrac_transport/scheme_policy.hpp"

namespace {
using Error = wfrac_transport::TransportError;
using Response = wfrac_transport::TransportResponse;
using Scheme = wfrac_transport::TransportScheme;

static_assert(HTTP_PARSER_VERSION_MAJOR == 2 && HTTP_PARSER_VERSION_MINOR == 7 &&
              HTTP_PARSER_VERSION_PATCH == 0, "D1 requires pinned parser v2.7.0");

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct Clock {
  std::int64_t now = 0;
  bool advance(std::int64_t elapsed) {
    if (elapsed < 0 || now < 0 || now > std::numeric_limits<std::int64_t>::max() - elapsed)
      return false;
    now += elapsed; return true;
  }
};
struct Budget {
  Clock& clock;
  std::int64_t last;
  std::int64_t end = 0;
  Error error = Error::none;
  Budget(Clock& c, std::uint32_t timeout) : clock(c), last(c.now) {
    if (c.now < 0 || timeout == 0 ||
        c.now > std::numeric_limits<std::int64_t>::max() - timeout) {
      error = Error::invalid_argument;
    } else {
      end = c.now + timeout;
    }
  }
  std::int64_t remaining() {
    if (error != Error::none) return 0;
    if (clock.now < last) { error = Error::io_failure; return 0; }
    last = clock.now;
    if (clock.now >= end) { error = Error::timeout; return 0; }
    return end - clock.now;
  }
};

// Private test-only policy surrounding the *real* upstream response parser.
// Conservative duplicate framing / no-upgrade / exactly one final response.
struct Framing {
  http_parser parser{};
  http_parser_settings settings{};
  std::string field, value, body;
  bool reading_value = false;
  bool headers_done = false;
  bool interim = false;
  bool final_complete = false;
  bool eof_seen = false;
  int status = 0;
  unsigned messages = 0;
  unsigned cl_count = 0, te_count = 0;
  std::size_t framing_bytes = 0;
  std::size_t framing_limit;
  std::size_t body_limit;
  Error error = Error::none;
  bool fail_body_callback = false;
  int header_callback_mode = 0;  // Explicit probes of SDK special return values.

  explicit Framing(std::size_t body_cap = 32, std::size_t framing_cap = 512)
      : framing_limit(framing_cap), body_limit(body_cap) {
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = this;
    settings.on_message_begin = begin;
    settings.on_header_field = header_field;
    settings.on_header_value = header_value;
    settings.on_headers_complete = headers;
    settings.on_body = data;
    settings.on_message_complete = complete;
  }
  static Framing& self(http_parser* p) { return *static_cast<Framing*>(p->data); }
  int reject(Error e) { if (error == Error::none) error = e; return -1; }
  static std::string normalized(std::string s) {
    for (char& ch : s) if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + 32);
    const auto first = s.find_first_not_of(" \t");
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(" \t") - first + 1);
  }
  int finish_header() {
    if (field.empty()) return 0;
    const auto name = normalized(field);
    if (name == "content-length") {
      if (headers_done || ++cl_count != 1 || te_count != 0) return reject(Error::io_failure);
    } else if (name == "transfer-encoding") {
      if (headers_done || ++te_count != 1 || cl_count != 0 ||
          normalized(value) != "chunked") return reject(Error::io_failure);
    }
    field.clear(); value.clear(); reading_value = false;
    return 0;
  }
  static int begin(http_parser* p) {
    auto& f = self(p);
    if (f.final_complete || ++f.messages > 4) return f.reject(Error::io_failure);
    f.field.clear(); f.value.clear(); f.reading_value = false;
    f.headers_done = false; f.cl_count = 0; f.te_count = 0;
    return 0;
  }
  static int header_field(http_parser* p, const char* at, std::size_t n) {
    auto& f = self(p);
    if (f.reading_value && f.finish_header() != 0) return -1;
    f.field.append(at, n);
    return 0;
  }
  static int header_value(http_parser* p, const char* at, std::size_t n) {
    auto& f = self(p); f.reading_value = true; f.value.append(at, n); return 0;
  }
  static int headers(http_parser* p) {
    auto& f = self(p);
    if (f.finish_header() != 0) return -1; // NEVER use 1/2 to signal failure.
    f.headers_done = true;
    f.interim = p->status_code >= 100 && p->status_code < 200;
    if (f.header_callback_mode != 0) return f.header_callback_mode;
    if (p->status_code == 101 || p->upgrade) return f.reject(Error::io_failure);
    if (!f.interim) f.status = p->status_code;
    return 0;
  }
  static int data(http_parser* p, const char* at, std::size_t n) {
    auto& f = self(p);
    // Count only non-payload wire bytes towards the finite framing budget.
    // Includes status/headers, chunk metadata, trailers and separators.
    f.framing_bytes -= n;
    if (f.fail_body_callback) return f.reject(Error::io_failure);
    if (f.interim || n > f.body_limit - f.body.size())
      return f.reject(Error::response_too_large);
    f.body.append(at, n);
    return 0;
  }
  static int complete(http_parser* p) {
    auto& f = self(p);
    if (f.finish_header() != 0) return -1;
    if (!f.interim) f.final_complete = true;
    return 0;
  }
  Error feed(const std::string& bytes) {
    if (error != Error::none) return error;
    if (eof_seen && !bytes.empty()) { reject(Error::io_failure); return error; }
    // One-byte delivery deliberately stresses actual SDK callback fragmentation.
    for (const char ch : bytes) {
      if (final_complete) { reject(Error::io_failure); break; }
      ++framing_bytes;
      const auto consumed = http_parser_execute(&parser, &settings, &ch, 1);
      if (HTTP_PARSER_ERRNO(&parser) != HPE_OK || consumed != 1) {
        reject(Error::io_failure); break;
      }
      if (framing_bytes > framing_limit) { reject(Error::io_failure); break; }
    }
    return error;
  }
  Error eof() {
    if (error != Error::none) return error;
    eof_seen = true;
    http_parser_execute(&parser, &settings, nullptr, 0);
    if (HTTP_PARSER_ERRNO(&parser) != HPE_OK || !final_complete)
      reject(Error::io_failure);
    return error;
  }
};

enum class Life { active, terminal_decided, stopping, quiescent, released };
struct Owner {
  Life state = Life::active;
  unsigned in_flight = 0;
  unsigned socket_closes = 0, tls_frees = 0, parser_frees = 0;
  unsigned accepted_callbacks = 0, rejected_callbacks = 0;
  bool socket_closed = false, failed_close = false;
  std::array<std::uint8_t, 8>* caller_buffer = nullptr;
  std::vector<std::string> events;
  bool callback() {
    if (state != Life::active) { ++rejected_callbacks; return false; }
    ++accepted_callbacks;
    if (caller_buffer) (*caller_buffer)[0] = 42;
    return true;
  }
  void terminal() {
    require(state == Life::active, "terminal transition must be unique");
    state = Life::terminal_decided; events.push_back("terminal");
  }
  bool stop(Clock& clock, std::int64_t latency, bool close_ok) {
    require(state == Life::terminal_decided || state == Life::stopping,
            "invalid stop transition");
    state = Life::stopping;
    if (in_flight != 0) { events.push_back("quarantined"); return false; }
    // Synthetic model latency only. Actual close timing remains UNPROVEN.
    if (!clock.advance(latency)) { failed_close = true; return false; }
    if (failed_close) return false;  // No blind second close after an uncertain failure.
    if (!socket_closed) {
      ++socket_closes; events.push_back("socket-stop");
      if (!close_ok) { failed_close = true; events.push_back("quarantined"); return false; }
      socket_closed = true;
    }
    state = Life::quiescent; events.push_back("quiescent");
    return true;
  }
  bool release() {
    if (state != Life::quiescent) return false;
    ++tls_frees; events.push_back("tls-free");
    ++parser_frees; events.push_back("parser-free");
    caller_buffer = nullptr;
    state = Life::released; events.push_back("released");
    return true;
  }
  bool may_start_next() const { return state == Life::released; }
};

// No ITransport implementation: this is a fake attempt with synthetic I/O events.
struct Attempt {
  Clock clock;
  Budget budget;
  Framing framing;
  Owner owner;
  Response& output;
  Error error = Error::none;
  unsigned connect_attempts = 0, writes = 0, effective_io = 0;
  std::vector<std::int64_t> remaining_at_io;
  Scheme scheme = Scheme::https;
  bool decided = false;
  explicit Attempt(Response& out, std::uint32_t timeout = 10, std::size_t cap = 32)
      : budget(clock, timeout), framing(cap), output(out) { output = {}; }
  bool checkpoint() {
    if (decided) return false;
    if (budget.remaining() == 0) { decide(budget.error); return false; }
    return true;
  }
  void decide(Error e) {
    if (decided) return;
    decided = true; error = e; output = {}; owner.terminal();
  }
  bool phase(std::int64_t duration, const std::string& progress) {
    if (!checkpoint()) return false;
    if (progress == "connect" && connect_attempts != 0) {
      decide(Error::io_failure); return false; // No reconnect within this fake attempt.
    }
    remaining_at_io.push_back(budget.remaining());
    ++effective_io;
    if (progress == "connect") ++connect_attempts;
    if (progress == "write") ++writes;
    if (!clock.advance(duration)) { decide(Error::io_failure); return false; }
    return checkpoint(); // Readiness/retry hints do not reset the deadline.
  }
  void receive(const std::string& bytes, std::int64_t elapsed = 0) {
    if (!checkpoint()) return;
    ++effective_io;
    if (!clock.advance(elapsed)) { decide(Error::io_failure); return; }
    if (!checkpoint()) return;
    if (!owner.callback()) { decide(Error::io_failure); return; }
    const auto e = framing.feed(bytes);
    if (e != Error::none) decide(e);
  }
  // These are controlled fake verification outcomes, not a TLS handshake.
  void tls_result(bool chain_ok, bool name_ok, bool sni_binding_ok, bool protocol_ok) {
    if (!checkpoint()) return;
    if (!chain_ok || !name_ok || !sni_binding_ok) decide(Error::tls_identity_failure);
    else if (!protocol_ok) decide(Error::tls_failure);
  }
  void finish(bool eof, std::int64_t cleanup = 0, bool close_ok = true) {
    if (!decided && checkpoint()) {
      Error e = eof ? framing.eof() : framing.error;
      if (e == Error::none && !framing.final_complete) e = Error::io_failure;
      decide(e);
    }
    if (!owner.stop(clock, cleanup, close_ok)) {
      if (error == Error::none) error = Error::io_failure;
      output = {}; return;
    }
    require(owner.release(), "quiescence precedes release");
    if (error == Error::none) output = {framing.status, framing.body.size()};
    else if (error == Error::response_too_large) output = {framing.status, 0};
  }
};

std::string cl(const std::string& body = "abc", int status = 200) {
  return "HTTP/1.1 " + std::to_string(status) + " Test\r\nContent-Length: " +
         std::to_string(body.size()) + "\r\n\r\n" + body;
}

using Test = std::pair<const char*, void(*)()>;
std::vector<Test> tests;
void add(const char* name, void(*run)()) { tests.emplace_back(name, run); }

void parser_tests() {
  add("parser/content-length exact + fragmented callbacks", [] {
    Framing f; require(f.feed(cl()) == Error::none, "CL parse");
    require(f.eof() == Error::none && f.body == "abc" && f.status == 200, "CL complete");
  });
  add("parser/content-length truncated EOF", [] {
    Framing f; f.feed("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nabc");
    require(f.eof() == Error::io_failure && !f.final_complete, "short CL rejected");
  });
  add("parser/chunked with fragmented trailers", [] {
    Framing f; f.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                     "2\r\nab\r\n1\r\nc\r\n0\r\nX-Test: value\r\n\r\n");
    require(f.eof() == Error::none && f.body == "abc", "chunk/trailer complete");
  });
  add("parser/truncated chunk + malformed chunk", [] {
    Framing short_chunk; short_chunk.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nab");
    require(short_chunk.eof() == Error::io_failure, "short chunk");
    Framing bad; require(bad.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\n")
                            == Error::io_failure, "bad chunk");
  });
  add("parser/EOF delimiter waits for actual EOF", [] {
    Framing f; f.feed("HTTP/1.0 200 OK\r\n\r\nabc");
    require(!f.final_complete && f.body == "abc", "not complete before EOF");
    require(f.eof() == Error::none && f.final_complete, "EOF completes");
  });
  add("parser/100 + 103 + one final response", [] {
    Framing f; f.feed("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 103 Early Hints\r\nX: y\r\n\r\n" + cl());
    require(f.eof() == Error::none && f.messages == 3 && f.body == "abc", "1xx then final");
  });
  add("parser/informational only + bounded informational flood", [] {
    Framing f; f.feed("HTTP/1.1 100 Continue\r\n\r\n");
    require(f.eof() == Error::io_failure, "interim not final");
    Framing flood; for (int i = 0; i < 5; ++i) flood.feed("HTTP/1.1 100 Continue\r\n\r\n");
    require(flood.error == Error::io_failure, "bounded interims");
  });
  add("parser/upgrade forbidden", [] {
    Framing f; require(f.feed("HTTP/1.1 101 Switching Protocols\r\nUpgrade: x\r\nConnection: upgrade\r\n\r\n")
                         == Error::io_failure, "no upgrade");
  });
  add("parser/zero messages + partial header", [] {
    Framing empty; require(empty.eof() == Error::io_failure, "empty input");
    Framing partial; partial.feed("HTTP/1.1 200 OK\r\nContent-Len");
    require(partial.eof() == Error::io_failure, "partial headers");
  });
  add("parser/multiple final responses + trailing bytes", [] {
    Framing f; require(f.feed(cl() + cl()) == Error::io_failure, "multiple finals");
    Framing tail; require(tail.feed(cl() + " ") == Error::io_failure, "trailing whitespace rejected");
  });
  add("parser/duplicate Content-Length same and conflicting", [] {
    for (const char* n : {"3", "4"}) {
      Framing f; require(f.feed(std::string("HTTP/1.1 200 OK\r\nContent-Length: 3\r\nContent-Length: ") +
                               n + "\r\n\r\nabc") == Error::io_failure, "duplicate CL");
    }
  });
  add("parser/TE-CL conflicts both orders + duplicate TE", [] {
    for (const auto& fields : {
        "Content-Length: 3\r\nTransfer-Encoding: chunked\r\n",
        "Transfer-Encoding: chunked\r\nContent-Length: 3\r\n",
        "Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n"}) {
      Framing f; require(f.feed(std::string("HTTP/1.1 200 OK\r\n") + fields + "\r\n")
                           == Error::io_failure, "ambiguous framing");
    }
  });
  add("parser/framing fields forbidden in trailers", [] {
    Framing f; require(f.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                             "1\r\na\r\n0\r\nContent-Length: 1\r\n\r\n")
                         == Error::io_failure, "trailer framing mutation");
  });
  add("parser/unsupported transfer coding fails closed", [] {
    Framing f; require(f.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n")
                         == Error::io_failure, "unsupported coding");
  });
  add("parser/finite framing budget includes trailer bytes", [] {
    Framing header(32, 50); require(header.feed("HTTP/1.1 200 OK\r\nX: " + std::string(80, 'a'))
                                       == Error::io_failure, "header bounded");
    Framing trailer(32, 100); require(trailer.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                                               "0\r\nX: " + std::string(120, 'a'))
                                        == Error::io_failure, "trailer bounded");
  });
  add("parser/body exact fit and overflow", [] {
    Framing exact(3); require(exact.feed(cl()) == Error::none && exact.eof() == Error::none, "exact fit");
    Framing overflow(2); require(overflow.feed(cl()) == Error::response_too_large, "overflow not parser generic");
  });
  add("parser/body callback failure propagates SDK errno", [] {
    Framing f; f.fail_body_callback = true; f.feed(cl());
    require(f.error == Error::io_failure && HTTP_PARSER_ERRNO(&f.parser) == HPE_CB_body,
            "real parser callback failure");
  });
  add("parser/header callback -1 is abort, 1/2 are NOT abort", [] {
    Framing abort; abort.header_callback_mode = -1; abort.feed(cl(""));
    require(HTTP_PARSER_ERRNO(&abort.parser) == HPE_CB_headers_complete, "-1 abort");
    for (int mode : {1, 2}) {
      Framing skip; skip.header_callback_mode = mode;
      const auto response = cl(); skip.feed(response.substr(0, response.size() - 3));
      require(HTTP_PARSER_ERRNO(&skip.parser) == HPE_OK && skip.final_complete,
              "1/2 special skip-body success, not failure");
      require((skip.parser.flags & F_SKIPBODY) != 0 &&
              (mode != 2 || skip.parser.upgrade != 0), "SDK special flags");
    }
  });
  add("parser/no body after EOF", [] {
    Framing f; f.feed("HTTP/1.0 200 OK\r\n\r\nabc"); f.eof();
    require(f.feed("d") == Error::io_failure, "no post-EOF bytes");
  });
}

void contract_tests() {
  add("L1/single budget across all five phases", [] {
    Response r{999, 999}; Attempt a(r, 11);
    require(r.body_size == 0 && r.http_status == 0, "entry reset");
    for (const char* phase : {"connect", "tls", "write", "headers", "body"})
      require(a.phase(2, phase), "phase remains in shared budget");
    require(a.budget.remaining() == 1 && a.connect_attempts == 1, "no reset");
    require(!a.phase(1, "body") && a.error == Error::timeout, "aggregate boundary");
  });
  add("L1/slow header and body trickle cannot refresh deadline", [] {
    Response r; Attempt a(r);
    const auto response = cl();
    for (int i = 0; i < 10; ++i) a.receive(response.substr(static_cast<std::size_t>(i), 1), 1);
    require(a.error == Error::timeout && r.body_size == 0, "header trickle timeout");
    Response r2; Attempt b(r2);
    b.receive("HTTP/1.1 200 OK\r\nContent-Length: 20\r\n\r\n");
    for (int i = 0; i < 10; ++i) b.receive("a", 1);
    require(b.error == Error::timeout && r2.body_size == 0, "body trickle timeout");
  });
  add("L1/EINTR EAGAIN WANT_READ WANT_WRITE recompute budget", [] {
    for (const char* hint : {"EINTR", "EAGAIN", "WANT_READ", "WANT_WRITE", "connect-ready"}) {
      Response r; Attempt a(r, 5);
      require(a.phase(4, hint), "progress hint before deadline");
      require(!a.phase(1, hint) && a.error == Error::timeout, "hint at exhaustion");
      require(a.remaining_at_io == std::vector<std::int64_t>{5, 1}, "remaining wait budget recomputed");
      const auto before = a.effective_io; a.phase(0, "write");
      require(a.effective_io == before, "no effective I/O after exhaustion");
    }
  });
  add("L1/zero negative-clock overflow regression wrap", [] {
    Clock c; Budget zero(c, 0); require(zero.error == Error::invalid_argument, "zero budget");
    Clock negative{-1}; Budget neg(negative, 10); require(neg.error == Error::invalid_argument, "negative clock");
    Clock high{std::numeric_limits<std::int64_t>::max() - 2}; Budget over(high, 3);
    require(over.error == Error::invalid_argument, "deadline overflow");
    Clock backwards{5}; Budget regression(backwards, 10); backwards.now = 4;
    require(regression.remaining() == 0 && regression.error == Error::io_failure, "clock regression");
    Clock wrap{std::numeric_limits<std::int64_t>::max() - 10}; Budget b(wrap, 5); wrap.now = 0;
    require(b.remaining() == 0 && b.error == Error::io_failure, "clock wrap");
  });
  add("L1/nonpreemptible operation counterexample", [] {
    Response r; Attempt a(r, 10); a.phase(17, "tls");
    require(a.error == Error::timeout && a.clock.now == 17, "post-check not hard return cap");
  });
  add("L2/success atomic only after quiescence + non-2xx", [] {
    Response r{999, 999}; Attempt a(r); a.receive(cl("abc", 429));
    require(r.body_size == 0 && r.http_status == 0, "no early publish");
    a.finish(true);
    require(a.error == Error::none && r.http_status == 429 && r.body_size == 3 &&
            a.owner.state == Life::released, "complete non-2xx transport success");
  });
  add("L2/truncated response clears sentinel and partial body", [] {
    Response r{999, 999}; Attempt a(r); a.receive("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nabc");
    a.finish(true); require(a.error == Error::io_failure && r.http_status == 0 && r.body_size == 0,
                            "partial response invalid");
  });
  add("L2/overflow status-only + real SchemePolicy evidence", [] {
    Response r{999, 999}; Attempt a(r, 10, 2); a.receive(cl());
    const auto before = a.effective_io; a.receive("never-drained"); a.finish(false);
    require(a.error == Error::response_too_large && r.http_status == 200 && r.body_size == 0 &&
            before == a.effective_io, "overflow stops with status only");
    wfrac_transport::SchemeCache cache;
    auto decision = wfrac_transport::observe_scheme_attempt(&cache, Scheme::https, a.error);
    require(cache.knowledge == wfrac_transport::SchemeKnowledge::https_confirmed &&
            decision.next_action == wfrac_transport::SchemeNextAction::stop, "existing overflow scheme evidence");
  });
  add("L2/timeout after possibly sent bytes no resend or downgrade", [] {
    Response r; Attempt a(r, 5); a.phase(1, "connect"); a.phase(4, "write");
    a.phase(0, "write"); a.phase(0, "connect"); a.finish(false);
    require(a.error == Error::timeout && a.writes == 1 && a.connect_attempts == 1 &&
            a.scheme == Scheme::https && r.body_size == 0, "no hidden retry/downgrade");
    Response r2; Attempt b(r2); b.phase(1, "connect"); b.phase(1, "connect");
    require(b.error == Error::io_failure && b.connect_attempts == 1, "no reconnect while still active");
  });
  add("L2/fake TLS chain hostname SNI errors remain identity failures", [] {
    for (const char* failed_check : {"chain", "hostname", "SNI-binding"}) {
      Response r{999, 999}; Attempt a(r); const std::string check(failed_check);
      a.phase(1, "connect");
      a.tls_result(check != "chain", check != "hostname", check != "SNI-binding", true);
      a.finish(false);
      require(a.error == Error::tls_identity_failure && r.body_size == 0 &&
              a.connect_attempts == 1 && a.scheme == Scheme::https, "identity fail closed");
      wfrac_transport::SchemeCache cache;
      require(wfrac_transport::observe_scheme_attempt(&cache, a.scheme, a.error).next_action ==
              wfrac_transport::SchemeNextAction::stop, "identity never downgrade");
    }
  });
  add("L2/primary TCP TLS invalid-input errors survive cleanup", [] {
    for (Error e : {Error::connection_failure, Error::tls_failure, Error::invalid_argument}) {
      Response r{999, 999}; Attempt a(r); a.decide(e); a.finish(false, 0, false);
      require(a.error == e && r.http_status == 0 && r.body_size == 0, "primary error preserved");
    }
  });
  add("L3/explicit lifecycle single owner teardown order", [] {
    Response r; Attempt a(r); a.receive(cl()); a.finish(true);
    require(a.owner.events == std::vector<std::string>{"terminal", "socket-stop", "quiescent",
                                                     "tls-free", "parser-free", "released"}, "order");
    require(a.owner.socket_closes == 1 && a.owner.tls_frees == 1 && a.owner.parser_frees == 1 &&
            a.owner.may_start_next(), "one owner no duplicate cleanup");
    require(!a.owner.release() && a.owner.tls_frees == 1, "double free blocked");
  });
  add("L3/in-flight callback quarantine then quiescence", [] {
    Owner owner; Clock c; owner.in_flight = 1; owner.terminal();
    require(!owner.stop(c, 0, true) && !owner.release() && !owner.may_start_next() &&
            owner.socket_closes == 0, "do not free in-flight state");
    owner.in_flight = 0;
    require(owner.stop(c, 0, true) && owner.release(), "later proven quiescence");
  });
  add("L3/late callbacks never touch caller after terminal/release", [] {
    Owner owner; Clock c; std::array<std::uint8_t, 8> caller{};
    owner.caller_buffer = &caller; require(owner.callback() && caller[0] == 42, "active callback");
    caller[0] = 77; owner.terminal();
    require(!owner.callback(), "terminal callback rejected");
    require(owner.stop(c, 0, true) && owner.release(), "release");
    require(!owner.callback() && owner.accepted_callbacks == 1 && owner.rejected_callbacks == 2,
            "late access blocked in synthetic owner");
    require(caller[0] == 77 && owner.caller_buffer == nullptr, "no stale pointer or post-terminal write");
  });
  add("L3/close failure not success, no release or conflicting attempt", [] {
    Response r{999, 999}; Attempt a(r); a.receive(cl()); a.finish(true, 0, false);
    require(a.error == Error::io_failure && r.http_status == 0 && r.body_size == 0 &&
            !a.owner.may_start_next() && !a.owner.release(), "uncertain close quarantined");
    require(!a.owner.stop(a.clock, 0, true) && a.owner.socket_closes == 1, "no blind second close");
  });
  add("L3/timeout cleanup overrun counterexample, safe return not hard deadline", [] {
    Response r; Attempt a(r, 10); a.phase(10, "write"); a.finish(false, 25);
    require(a.error == Error::timeout && a.clock.now == 35 && a.owner.state == Life::released &&
            r.body_size == 0, "safe cleanup exceeds I/O budget");
  });
  add("L3/complete-before-deadline cleanup later, report separate clocks", [] {
    Response r; Attempt a(r, 10); a.receive(cl(), 2); a.finish(true, 25);
    require(a.error == Error::none && a.clock.now == 27 && a.owner.state == Life::released &&
            r.body_size == 3, "L1 completion differs from caller return/cleanup time");
  });
}
} // namespace

int main() {
  parser_tests(); contract_tests();
  std::cout << "D1 OFFLINE TEST-ONLY; actual upstream parser version=" << http_parser_version()
            << "; no SDK timing/TLS/network/cleanup guarantee\n";
  unsigned passed = 0;
  for (const auto& test : tests) {
    try { test.second(); ++passed; std::cout << "PASS " << test.first << '\n'; }
    catch (const std::exception& e) {
      std::cerr << "FAIL " << test.first << ": " << e.what() << '\n';
      return 1; // STOP at first failed contract; no production workaround.
    }
  }
  std::cout << "D1: " << passed << '/' << tests.size() << " cases PASS\n";
}
