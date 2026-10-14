// A3 synthetic DNS callback-owner model only.
// It does not call lwIP, resolve a hostname, open sockets, run TLS, or prove
// ESP-IDF callback/quiescence timing. The callback-return event is controlled
// by the fake scheduler and must not be mistaken for an SDK guarantee.
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

enum class Start { immediate_ok, in_progress, err_mem, err_arg, dispatch_failed };
enum class State { free, pending, callback_active, result_ready, timed_out, stopping };
enum class Result { none, success, failure, timeout };

template <std::size_t SlotCount, std::size_t TokenCount>
class Owner {
 public:
  struct Context {
    Owner* owner = nullptr;
    std::size_t slot = 0;
    std::uint64_t generation = 0;
    bool allocated = false;
    bool active = false;
    bool callback_seen = false;
  };

  struct Started {
    Result result = Result::none;
    Context* context = nullptr;
    bool registered = false;
  };

  Started start(const std::string& hostname, std::uint64_t now,
                std::uint64_t timeout, Start start_result) {
    ++start_calls;
    if (hostname.empty() || timeout == 0 || now > UINT64_MAX - timeout)
      return {Result::failure, nullptr, false};
    if (stopping_) return {Result::failure, nullptr, false};
    if (start_result == Start::immediate_ok) return {Result::success, nullptr, false};
    if (start_result == Start::err_mem || start_result == Start::err_arg ||
        start_result == Start::dispatch_failed)
      return {Result::failure, nullptr, false};

    std::size_t slot_index = SlotCount;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
      if (slots_[i].state == State::free) { slot_index = i; break; }
    }
    if (slot_index == SlotCount) { ++slot_exhaustions; return {}; }

    Context* token = nullptr;
    for (auto& candidate : contexts_) {
      if (!candidate.allocated) { token = &candidate; break; }
    }
    if (token == nullptr) { ++token_exhaustions; return {}; }

    auto& slot = slots_[slot_index];
    ++next_generation_;
    slot = Slot{};
    slot.hostname = hostname;
    slot.deadline = now + timeout;
    slot.generation = next_generation_;
    slot.state = State::pending;
    token->owner = this;
    token->slot = slot_index;
    token->generation = slot.generation;
    token->allocated = true;
    token->active = true;
    token->callback_seen = false;
    ++registrations;
    return {Result::none, token, true};
  }

  bool callback_enter(Context* context) {
    auto* slot = checked_slot(context);
    if (slot == nullptr || !context->active || context->callback_seen ||
        slot->generation != context->generation ||
        (slot->state != State::pending && slot->state != State::timed_out &&
         slot->state != State::stopping)) {
      ++rejected_callbacks;
      return false;
    }
    context->callback_seen = true;
    slot->in_callback = true;
    if (slot->state == State::pending) slot->state = State::callback_active;
    ++callbacks_entered;
    return true;
  }

  bool callback_deliver(Context* context, Result result, std::uint64_t now) {
    auto* slot = checked_slot(context);
    if (slot == nullptr || !slot->in_callback ||
        slot->generation != context->generation) return false;
    if (slot->state == State::callback_active && now >= slot->deadline) {
      slot->state = State::timed_out;
      slot->result = Result::timeout;
      return false;
    }
    if (slot->state == State::callback_active && now < slot->deadline &&
        !stopping_) {
      slot->state = State::result_ready;
      slot->result = result == Result::success ? Result::success : Result::failure;
      return true;
    }
    // A late callback is observed and discarded; caller memory is never stored.
    return false;
  }

  bool callback_return(Context* context) {
    auto* slot = checked_slot(context);
    if (slot == nullptr || !context->active || !slot->in_callback) return false;
    slot->in_callback = false;
    context->active = false;  // Fake scheduler's post-callback-return boundary.
    ++callbacks_returned;
    if (slot->state == State::timed_out || slot->state == State::stopping) {
      slot->state = State::free;
      ++quiescent_releases;
    }
    return true;
  }

  bool expire(std::size_t slot_index, std::uint64_t now) {
    if (slot_index >= SlotCount) return false;
    auto& slot = slots_[slot_index];
    if (now < slot.deadline ||
        (slot.state != State::pending && slot.state != State::callback_active))
      return false;
    slot.state = State::timed_out;
    slot.result = Result::timeout;
    return true;
  }

  Result consume(std::size_t slot_index) {
    if (slot_index >= SlotCount) return Result::none;
    auto& slot = slots_[slot_index];
    if (slot.state != State::result_ready || slot.in_callback) return Result::none;
    const auto result = slot.result;
    slot.state = State::free;
    return result;
  }

  void clear_dns_cache_notification() { ++cache_clear_notifications; }

  void shutdown() {
    stopping_ = true;
    for (auto& slot : slots_) {
      if (slot.state == State::pending || slot.state == State::callback_active)
        slot.state = State::stopping;
    }
  }

  State state(std::size_t i) const { return slots_.at(i).state; }
  std::uint64_t generation(std::size_t i) const { return slots_.at(i).generation; }
  bool callback_in_progress(std::size_t i) const { return slots_.at(i).in_callback; }
  const auto& contexts() const { return contexts_; }
  std::size_t retained() const {
    std::size_t count = 0;
    for (const auto& item : slots_) if (item.state != State::free) ++count;
    return count;
  }
  bool stopping() const { return stopping_; }

  unsigned start_calls = 0;
  unsigned registrations = 0;
  unsigned callbacks_entered = 0;
  unsigned callbacks_returned = 0;
  unsigned rejected_callbacks = 0;
  unsigned quiescent_releases = 0;
  unsigned cache_clear_notifications = 0;
  unsigned slot_exhaustions = 0;
  unsigned token_exhaustions = 0;

 private:
  struct Slot {
    State state = State::free;
    std::string hostname;
    std::uint64_t deadline = 0;
    std::uint64_t generation = 0;
    Result result = Result::none;
    bool in_callback = false;
  };

  Slot* checked_slot(Context* context) {
    if (context == nullptr || context->owner != this) return nullptr;
    for (auto& token : contexts_) if (&token == context) {
      if (!token.allocated || token.slot >= SlotCount) return nullptr;
      return &slots_[token.slot];
    }
    return nullptr;
  }

  std::array<Slot, SlotCount> slots_{};
  // Callback tokens are fixed-capacity and never reused: stale tokens remain
  // readable and fail generation/active checks instead of aliasing new work.
  std::array<Context, TokenCount> contexts_{};
  std::uint64_t next_generation_ = 0;
  bool stopping_ = false;
};

struct Case { const char* name; void (*run)(); };
std::vector<Case> cases;
void add(const char* name, void (*run)()) { cases.push_back({name, run}); }

struct FakeCloseOwner {
  bool socket_owned = true;
  bool released = false;
  void close_completed(int error) {
    if (error == 0) {
      socket_owned = false;
      released = true;
    }
  }
};

using SmallOwner = Owner<2, 4>;

void register_cases() {
  add("ERR_OK cached/immediate uses no callback slot", [] {
    SmallOwner owner;
    const auto started = owner.start("rac.local", 0, 10, Start::immediate_ok);
    require(started.result == Result::success && !started.registered && owner.retained() == 0,
            "immediate result must not register callback");
  });
  add("ERR_INPROGRESS retains stable owner context", [] {
    SmallOwner owner;
    const auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(started.registered && started.context != nullptr && owner.retained() == 1,
            "pending registration retained");
    require(started.context->owner == &owner && started.context->slot == 0,
            "context belongs to fixed owner storage");
  });
  add("immediate ERR_MEM releases without callback registration", [] {
    SmallOwner owner;
    const auto started = owner.start("rac.local", 0, 10, Start::err_mem);
    require(started.result == Result::failure && !started.registered &&
            owner.retained() == 0 && owner.registrations == 0, "ERR_MEM is terminal");
  });
  add("immediate ERR_ARG releases without callback registration", [] {
    SmallOwner owner;
    const auto started = owner.start("rac.local", 0, 10, Start::err_arg);
    require(started.result == Result::failure && !started.registered &&
            owner.retained() == 0 && owner.registrations == 0, "ERR_ARG is terminal");
  });
  add("failed core dispatch creates no callback owner", [] {
    SmallOwner owner;
    const auto started = owner.start("rac.local", 0, 10, Start::dispatch_failed);
    require(started.result == Result::failure && !started.registered &&
            owner.retained() == 0 && owner.registrations == 0, "dispatch failure is terminal");
  });
  add("callback before deadline publishes result after callback return", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(started.context), "callback enters");
    require(owner.callback_deliver(started.context, Result::success, 9), "early result accepted");
    require(owner.consume(0) == Result::none, "do not consume while callback active");
    require(owner.callback_return(started.context), "callback return boundary");
    require(owner.consume(0) == Result::success, "early result delivered to waiter");
  });
  add("callback after deadline success is discarded", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(owner.expire(0, 10), "deadline observed");
    require(owner.callback_enter(started.context), "late callback enters retained context");
    require(!owner.callback_deliver(started.context, Result::success, 11), "late success ignored");
    require(owner.callback_return(started.context) && owner.retained() == 0,
            "release only at fake quiescence boundary");
  });
  add("callback after deadline failure is discarded", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(owner.expire(0, 10) && owner.callback_enter(started.context), "late callback enters");
    require(!owner.callback_deliver(started.context, Result::failure, 12), "late failure ignored");
    require(owner.callback_return(started.context) && owner.retained() == 0,
            "quiescence required");
  });
  add("callback observes deadline even before owner timeout checkpoint", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(started.context), "callback active");
    require(!owner.callback_deliver(started.context, Result::success, 10) &&
            owner.state(0) == State::timed_out,
            "callback cannot publish result after deadline");
    require(owner.callback_return(started.context) && owner.retained() == 0,
            "timed-out slot releases at fake quiescence boundary");
  });
  add("callback-in-progress deadline race blocks reuse", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(started.context), "callback active");
    require(owner.expire(0, 10) && owner.retained() == 1, "timeout does not free active callback");
    const auto next = owner.start("other.local", 10, 10, Start::in_progress);
    require(next.registered && next.context->slot == 1, "other slot only");
    require(owner.callback_return(started.context) && owner.retained() == 1,
            "timed-out slot releases only after callback return");
  });
  add("late callback never accesses caller buffer", [] {
    SmallOwner owner;
    std::array<unsigned char, 4> caller_buffer{{7, 7, 7, 7}};
    auto started = owner.start("rac.local", 0, 1, Start::in_progress);
    require(owner.expire(0, 1), "caller timeout");
    caller_buffer.fill(0);  // Simulated caller frame/storage has moved on.
    require(owner.callback_enter(started.context), "late callback enters");
    require(!owner.callback_deliver(started.context, Result::success, 2), "no late publication");
    require(owner.callback_return(started.context), "callback quiescent");
    require(caller_buffer == std::array<unsigned char, 4>{{0, 0, 0, 0}},
            "callback context holds no caller-buffer reference");
  });
  add("callback context does not depend on caller stack object", [] {
    SmallOwner owner;
    SmallOwner::Context* context = nullptr;
    {
      struct CallerFrame { int marker = 42; } caller;
      auto started = owner.start("rac.local", 0, 10, Start::in_progress);
      context = started.context;
      require(caller.marker == 42 && context != nullptr, "caller frame exists only in test scope");
    }
    require(owner.callback_enter(context) &&
            owner.callback_deliver(context, Result::failure, 1) &&
            owner.callback_return(context), "owner context survives caller scope in model");
  });
  add("single-use callback token rejects duplicate invocation", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(started.context), "first callback accepted");
    require(!owner.callback_enter(started.context), "duplicate callback rejected");
    require(owner.callback_deliver(started.context, Result::success, 1) &&
            owner.callback_return(started.context), "first callback completes");
    require(!owner.callback_return(started.context) && owner.rejected_callbacks == 1 &&
            owner.quiescent_releases == 0, "duplicate callback/free is not repeated");
  });
  add("retired stale token cannot complete reused slot generation", [] {
    Owner<1, 3> owner;
    auto old = owner.start("old.local", 0, 10, Start::in_progress);
    const auto old_generation = old.context->generation;
    require(owner.callback_enter(old.context) &&
            owner.callback_deliver(old.context, Result::success, 1) &&
            owner.callback_return(old.context) && owner.consume(0) == Result::success,
            "first generation completes and slot is reused");
    auto fresh = owner.start("new.local", 2, 10, Start::in_progress);
    require(fresh.context->generation > old_generation && fresh.context->slot == 0,
            "new generation uses released slot");
    require(!owner.callback_enter(old.context), "old stable token rejected");
    require(owner.generation(0) == fresh.context->generation &&
            owner.state(0) == State::pending, "stale token cannot alter new work");
  });
  add("DNS cache-clear notification does not cancel registered callback", [] {
    SmallOwner owner;
    auto started = owner.start("rac.local", 0, 10, Start::in_progress);
    owner.clear_dns_cache_notification();
    require(owner.cache_clear_notifications == 1 && owner.retained() == 1 &&
            owner.state(0) == State::pending, "cache notification is not cancellation");
    require(owner.callback_enter(started.context) &&
            owner.callback_deliver(started.context, Result::success, 1) &&
            owner.callback_return(started.context) && owner.consume(0) == Result::success,
            "registration remains live");
  });
  add("concurrent duplicate hostnames have distinct registrations", [] {
    SmallOwner owner;
    auto first = owner.start("same.local", 0, 10, Start::in_progress);
    auto second = owner.start("same.local", 0, 10, Start::in_progress);
    require(first.registered && second.registered && first.context != second.context &&
            first.context->slot != second.context->slot && owner.registrations == 2,
            "no implicit coalescing or shared callback owner");
  });
  add("fixed slot limit reports resource exhaustion", [] {
    SmallOwner owner;
    auto first = owner.start("a.local", 0, 10, Start::in_progress);
    auto second = owner.start("b.local", 0, 10, Start::in_progress);
    const auto third = owner.start("c.local", 0, 10, Start::in_progress);
    require(first.registered && second.registered && !third.registered &&
            owner.slot_exhaustions == 1 && owner.retained() == 2,
            "bounded visible slot exhaustion");
  });
  add("never-called-back registration stays safely retained", [] {
    SmallOwner owner;
    const auto started = owner.start("silent.local", 0, 1, Start::in_progress);
    require(owner.expire(0, 100), "owner observes timeout after stall");
    require(started.context->active && owner.retained() == 1 &&
            owner.state(0) == State::timed_out,
            "no callback means context/slot remain retained");
    const auto second = owner.start("next.local", 100, 10, Start::in_progress);
    const auto third = owner.start("last.local", 100, 10, Start::in_progress);
    require(second.registered && second.context->slot == 1 && !third.registered &&
            owner.slot_exhaustions == 1,
            "retained slot is not reused; exhaustion is fail closed");
  });
  add("scheduler stall has no autonomous timeout guarantee", [] {
    SmallOwner owner;
    owner.start("stall.local", 0, 1, Start::in_progress);
    // Advancing fake wall time alone has no effect; only an owner checkpoint runs.
    require(owner.state(0) == State::pending && owner.retained() == 1,
            "no timer/worker implied by elapsed time");
    require(owner.expire(0, 100) && owner.state(0) == State::timed_out,
            "bounded decision only after explicit checkpoint");
  });
  add("one start call makes no retry or server failover", [] {
    SmallOwner owner;
    owner.start("primary.local", 0, 1, Start::in_progress);
    owner.expire(0, 1);
    require(owner.start_calls == 1 && owner.registrations == 1,
            "model has no retry/failover loop");
  });
  add("shutdown before callback retains context until callback quiescence", [] {
    SmallOwner owner;
    auto started = owner.start("shutdown.local", 0, 10, Start::in_progress);
    owner.shutdown();
    require(owner.stopping() && owner.retained() == 1 && started.context->active,
            "shutdown does not free registered context");
    require(owner.callback_enter(started.context), "late callback can safely observe shutdown");
    require(!owner.callback_deliver(started.context, Result::success, 1), "shutdown result ignored");
    require(owner.retained() == 1 && owner.callback_return(started.context) &&
            owner.retained() == 0, "release occurs only after fake quiescence");
  });
  add("shutdown during callback waits for in-flight callback return", [] {
    SmallOwner owner;
    auto started = owner.start("shutdown.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(started.context), "callback active");
    owner.shutdown();
    require(owner.retained() == 1 && owner.callback_in_progress(0),
            "no release while callback active");
    require(owner.callback_return(started.context) && owner.retained() == 0,
            "release only after fake return event");
  });
  add("shutdown after callback completion rejects new registration", [] {
    SmallOwner owner;
    auto started = owner.start("finished.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(started.context) &&
            owner.callback_deliver(started.context, Result::success, 1) &&
            owner.callback_return(started.context) && owner.consume(0) == Result::success,
            "registration reaches completed state before shutdown");
    owner.shutdown();
    require(owner.stopping() && owner.retained() == 0 &&
            !owner.start("after-shutdown.local", 2, 10, Start::in_progress).registered,
            "shutdown is terminal for new resolver work");
  });
  add("fixed callback-token pool exhaustion is explicit", [] {
    Owner<2, 1> owner;
    auto first = owner.start("first.local", 0, 10, Start::in_progress);
    require(owner.callback_enter(first.context) &&
            owner.callback_deliver(first.context, Result::success, 1) &&
            owner.callback_return(first.context) && owner.consume(0) == Result::success,
            "first token is retired, not reused");
    require(!owner.start("second.local", 2, 10, Start::in_progress).registered &&
            owner.token_exhaustions == 1, "bounded token capacity fails closed");
  });
  add("TLS verification identity is logical hostname, not socket address", [] {
    const std::string logical = "rac.example.test";
    const std::string socket_address = "192.0.2.10";
    const auto policy = [&](const std::string& cert_name, const std::string& sni) {
      return cert_name == logical && sni == logical && socket_address != logical;
    };
    require(policy("rac.example.test", "rac.example.test"), "logical identity preserved");
    require(!policy("192.0.2.10", "rac.example.test"), "IP certificate identity rejected");
    require(!policy("rac.example.test", "192.0.2.10"), "IP SNI substitution rejected");
  });
  add("cleanup ERR_MEM counterexample quarantines owner", [] {
    FakeCloseOwner owner;
    owner.close_completed(-1);  // Synthetic ERR_MEM; not an lwIP invocation.
    require(owner.socket_owned && !owner.released,
            "failed close cannot claim quiescent/released");
  });
  add("unbounded close counterexample remains NOT_RUN, no false release", [] {
    FakeCloseOwner owner;
    // No close-return event is scheduled; this cannot simulate real elapsed time.
    require(owner.socket_owned && !owner.released,
            "without close completion resources remain quarantined");
  });
}
}  // namespace

int main() {
  register_cases();
  unsigned passed = 0;
  for (const auto& test : cases) {
    try {
      test.run();
      ++passed;
      std::cout << "PASS " << test.name << '\n';
    } catch (const std::exception& error) {
      std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
      return 1;
    }
  }
  std::cout << "A3 OFFLINE SYNTHETIC-ONLY: " << passed << '/' << cases.size()
            << " deterministic cases PASS; no lwIP/SDK/network/TLS/socket execution; "
               "caller-return, callback-quiescence, eventual reclamation and close timing UNPROVEN\n";
  return passed == cases.size() ? 0 : 1;
}
