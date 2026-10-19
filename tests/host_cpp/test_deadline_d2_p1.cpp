// D2-P1-T SYNTHETIC ONLY. No SDK/socket/TLS engine calls or threads.
// Models pinned ssl_write exact-argument and borrowed config/CA/BIO contracts;
// does not pretend these stand-ins execute mbedTLS or prove native quiescence.
#define WFRAC_D2_FIXTURES_ONLY
#include "test_deadline_d2_p0.cpp"
#undef WFRAC_D2_FIXTURES_ONLY

namespace {
namespace p1 {
enum class Result { want_read, want_write, positive };
struct Borrowed {
  d2::OwnerSlot slot;
  bool config = true, ca = true, bio = true, ssl = true, callback = false;
  bool released = false;
  Borrowed() { require(slot.start(), "one synthetic attempt admission"); }
  bool valid() const { return config && ca && bio && ssl && !released; }
  bool free_ssl() {
    if (callback || !ssl) return false;
    ssl = false; return true; // Only context disposition, NOT socket close.
  }
  void terminal() { if (slot.owner.state == Life::active) slot.owner.terminal(); }
  bool close(d2::Close witness) {
    terminal();
    if (ssl || callback) return false;
    return slot.owner.stop(witness);
  }
  bool release() {
    if (ssl || callback || released || !slot.owner.release()) return false;
    config = ca = bio = false; released = true; return true;
  }
  void masked_close_status() {
    terminal();
    auto& owner = slot.owner;
    require(!owner.close_attempted, "no duplicate synthetic close injection");
    owner.state = Life::stopping; owner.close_attempted = true; ++owner.close_calls;
    owner.unknown_fd = owner.quarantined = true; owner.close_ack = false;
    // Models a void helper/invalidated local handle, NOT a real socket failure.
  }
};
struct WriteWitness {
  Borrowed& owner;
  const unsigned char* base;
  std::size_t total, offset = 0;
  const unsigned char* retry_pointer = nullptr;
  std::size_t retry_length = 0;
  unsigned calls = 0;
  bool call(const unsigned char* pointer, std::size_t length, Result result, std::size_t written = 0) {
    if (!owner.valid() || !pointer || !length || offset >= total) return false;
    if (retry_pointer && (pointer != retry_pointer || length != retry_length)) return false;
    if (pointer != base + offset || length != total - offset) return false;
    if (result == Result::positive && (written == 0 || written > length)) return false;
    if (result != Result::positive && written != 0) return false;
    ++calls;
    if (result != Result::positive) { retry_pointer = pointer; retry_length = length; }
    else { offset += written; retry_pointer = nullptr; retry_length = 0; }
    return true;
  }
};
struct TimedReceive {
  std::vector<std::uint32_t> arguments;
  bool call(std::int64_t remaining) {
    // No 0 (potential indefinite receive), no maximum/infinite sentinel, no narrowing.
    if (remaining <= 0 || remaining >= std::numeric_limits<std::uint32_t>::max()) return false;
    arguments.push_back(static_cast<std::uint32_t>(remaining)); return true;
  }
};
void register_tests() {
  add("P1/WANT_READ retains exact pointer and length", [] {
    Borrowed owner; const std::array<unsigned char,4> bytes{{1,2,3,4}};
    WriteWitness w{owner,bytes.data(),bytes.size()};
    require(w.call(bytes.data(),4,Result::want_read) && w.call(bytes.data(),4,Result::positive,4) &&
            w.offset == 4 && owner.slot.attempts == 1, "same arguments retry, not second request");
  });
  add("P1/WANT_WRITE retains exact pointer and length", [] {
    Borrowed owner; const std::array<unsigned char,4> bytes{{1,2,3,4}};
    WriteWitness w{owner,bytes.data(),bytes.size()};
    require(w.call(bytes.data(),4,Result::want_write) && w.call(bytes.data(),4,Result::want_read) &&
            w.retry_pointer == bytes.data() && w.retry_length == 4, "cross-WANT exact argument retention");
  });
  add("P1/equal bytes at different address must be rejected", [] {
    Borrowed owner; const std::array<unsigned char,4> bytes{{1,2,3,4}}, copy=bytes;
    require(bytes == copy && bytes.data() != copy.data(), "negative really uses different address");
    WriteWitness w{owner,bytes.data(),bytes.size()};
    require(w.call(bytes.data(),4,Result::want_write) && !w.call(copy.data(),4,Result::positive,4) &&
            w.calls == 1 && w.offset == 0, "byte equality cannot replace pointer identity");
  });
  add("P1/same pointer changed retry length must be rejected", [] {
    Borrowed owner; const std::array<unsigned char,4> bytes{}; WriteWitness w{owner,bytes.data(),4};
    require(w.call(bytes.data(),4,Result::want_read) && !w.call(bytes.data(),3,Result::positive,3) &&
            w.calls == 1 && w.retry_length == 4, "same length required until positive return");
  });
  add("P1/positive short write alone advances pointer and remaining length", [] {
    Borrowed owner; const std::array<unsigned char,4> bytes{}; WriteWitness w{owner,bytes.data(),4};
    require(w.call(bytes.data(),4,Result::positive,1) &&
            w.call(bytes.data()+1,3,Result::want_write) &&
            !w.call(bytes.data(),4,Result::positive,4) &&
            w.call(bytes.data()+1,3,Result::positive,3) && w.offset == 4 &&
            !w.call(bytes.data(),4,Result::positive,4) && owner.slot.attempts == 1,
            "no rewind or extra physical attempt");
  });
  add("P1/early config CA BIO release blocks subsequent TLS call", [] {
    for (int field=0;field<3;++field) {
      Borrowed owner; const std::array<unsigned char,4> bytes{}; WriteWitness w{owner,bytes.data(),4};
      require(w.call(bytes.data(),4,Result::want_write), "pending TLS retry");
      if (field==0) owner.config=false;
      if (field==1) owner.ca=false;
      if (field==2) owner.bio=false;
      require(!w.call(bytes.data(),4,Result::positive,4) && w.calls == 1,
              "borrowed dependency must outlive active SSL context");
    }
  });
  add("P1/BIO context and write buffer remain stable across WANT", [] {
    Borrowed owner; const void* context=&owner; const std::array<unsigned char,4> bytes{};
    WriteWitness w{owner,bytes.data(),4};
    require(w.call(bytes.data(),4,Result::want_read) && context == &w.owner &&
            w.owner.config && w.owner.ca && w.owner.bio && w.call(bytes.data(),4,Result::positive,2),
            "retained exact BIO owner identity");
  });
  add("P1/in-flight callback forbids SSL or borrower release", [] {
    Borrowed owner; owner.callback=true; owner.slot.owner.in_flight=1;
    require(!owner.free_ssl() && !owner.release() && !owner.close(d2::Close::acknowledged) &&
            owner.config && owner.ca && owner.bio && !owner.slot.start(), "no early free or conflicting attempt");
    owner.callback=false; owner.slot.owner.in_flight=0;
    require(owner.free_ssl() && owner.close(d2::Close::acknowledged) && owner.release(),
            "only synthetic observed safe completion permits disposal");
  });
  add("P1/SSL free and close-notify status are not socket close proof", [] {
    Borrowed owner; require(owner.free_ssl(), "synthetic context free");
    const bool close_notify_returned_ok=true;
    require(close_notify_returned_ok && !owner.release() && owner.config && owner.ca && owner.bio &&
            !owner.slot.owner.close_ack, "no fabricated lower-layer witness");
  });
  add("P1/zero max negative and narrowing receive budgets fail closed", [] {
    TimedReceive recv;
    for (std::int64_t timeout : {std::int64_t{0},std::int64_t{-1},
        std::int64_t{std::numeric_limits<std::uint32_t>::max()},
        std::int64_t{std::numeric_limits<std::uint32_t>::max()}+1}) require(!recv.call(timeout), "invalid sentinel");
    Clock c; Budget zero(c,0); require(!recv.call(zero.remaining()) && recv.arguments.empty(), "no infinite receive on expired budget");
  });
  add("P1/WANT recomputes one budget and never passes zero to receive", [] {
    Clock c; Budget budget(c,2); TimedReceive recv;
    require(recv.call(budget.remaining()) && c.advance(1) && recv.call(budget.remaining()) &&
            c.advance(1) && !recv.call(budget.remaining()) &&
            recv.arguments == std::vector<std::uint32_t>{2,1}, "nonzero shared remaining only");
  });
  add("P1/masked close success cannot fabricate resource disposition", [] {
    Borrowed owner; require(owner.free_ssl(), "context freed, socket not yet acknowledged");
    owner.masked_close_status();
    const bool helper_returned_ok=true;
    require(helper_returned_ok && !owner.release() && !owner.close(d2::Close::acknowledged) &&
            !owner.slot.start() && owner.slot.owner.close_calls == 1 && owner.config && owner.ca && owner.bio,
            "unknown fd remains quarantined, no double close or fake witness");
  });
  add("P1/explicit failed close retains owner and blocks next attempt", [] {
    Borrowed owner; owner.free_ssl();
    require(!owner.close(d2::Close::error) && !owner.close(d2::Close::acknowledged) &&
            !owner.release() && !owner.slot.start() && owner.slot.owner.close_calls == 1,
            "later fabricated success cannot repair unknown close");
  });
  add("P1/positive borrower lifetime disposes once only after close witness", [] {
    Borrowed owner;
    require(!owner.release() && owner.free_ssl() && !owner.release() &&
            owner.close(d2::Close::acknowledged) && owner.release() && !owner.release() &&
            !owner.config && !owner.ca && !owner.bio && owner.slot.owner.close_calls == 1 &&
            owner.slot.owner.frees == 1, "one owner, no premature or duplicate disposal");
  });
}
}
} // namespace
int main() {
  parser_tests(); contract_tests(); require(tests.size()==36,"D1 inventory drift");
  d2::register_tests(); require(tests.size()==62,"D2-P0 inventory drift");
  p1::register_tests(); unsigned passed=0;
  std::cout << "Pinned parser version=" << http_parser_version() << "; host fake ownership only\n";
  for (const auto& test:tests) {
    try { test.second(); ++passed; std::cout << "PASS " << test.first << '\n'; }
    catch(const std::exception& e) { std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; return 1; }
    if(passed==36) std::cout << "D1: 36/36 cases PASS\n";
    if(passed==62) std::cout << "D2-P0-T: 26/26 SYNTHETIC_PASS\n";
  }
  std::cout << "D2-P1-T: " << passed-62 << '/' << tests.size()-62 << " SYNTHETIC_PASS\n"
            << "NOT_RUN: native TLS/BIO/socket execution, handshake/network/hardware; target compile/link reported separately\n";
}
