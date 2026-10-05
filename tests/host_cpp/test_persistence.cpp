#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <type_traits>

#include "wfrac_persistence/records.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {
using namespace wfrac_persistence;
using Scheme = wfrac_transport::SchemeKnowledge;
using Error = PersistenceError;

void check(bool condition) { if (!condition) std::abort(); }
void expect(Error actual, Error expected) { check(actual == expected); }

// Bounded, volatile test storage only; no durability or commit guarantees.
class FakePersistence final : public IPersistence {
 public:
  struct Slot {
    std::array<char, max_device_key_size + 1U> key{};
    RecordBuffer operator_id{};
    RecordBuffer scheme{};
  };
  std::array<Slot, 2> slots{};
  RecordBuffer checkpoint{};
  bool storage_failure = false;

  Error load_operator_id(DeviceKey key, char* out, std::size_t capacity,
                         std::size_t* size) override {
    if (size) *size = 0;
    if (out && capacity) out[0] = '\0';
    if (!valid_device_key(key) || !size || (!out && capacity)) return Error::invalid_argument;
    if (storage_failure) return Error::storage_failure;
    auto* slot = find(key);
    if (!slot || !slot->operator_id.size) return Error::not_found;
    return decode_operator_id(slot->operator_id, out, capacity, size);
  }
  Error store_operator_id(DeviceKey key, OperatorIdView value) override {
    if (!valid_device_key(key)) return Error::invalid_argument;
    RecordBuffer encoded;
    const auto error = encode_operator_id(value, &encoded);
    if (error != Error::none) return error;
    if (storage_failure) return Error::storage_failure;
    auto* slot = acquire(key);
    if (!slot) return Error::storage_failure;
    slot->operator_id = encoded;
    return Error::none;
  }
  Error erase_operator_id(DeviceKey key) override { return erase(key, false); }
  Error load_scheme(DeviceKey key, Scheme* value) override {
    if (value) *value = Scheme::unknown;
    if (!valid_device_key(key) || !value) return Error::invalid_argument;
    if (storage_failure) return Error::storage_failure;
    auto* slot = find(key);
    if (!slot || !slot->scheme.size) return Error::not_found;
    return decode_scheme(slot->scheme, value);
  }
  Error store_scheme(DeviceKey key, Scheme value) override {
    if (!valid_device_key(key)) return Error::invalid_argument;
    RecordBuffer encoded;
    const auto error = encode_scheme(value, &encoded);
    if (error != Error::none) return error;
    if (storage_failure) return Error::storage_failure;
    auto* slot = acquire(key);
    if (!slot) return Error::storage_failure;
    slot->scheme = encoded;
    return Error::none;
  }
  Error erase_scheme(DeviceKey key) override { return erase(key, true); }
  Error load_time_checkpoint(TimeCheckpoint* value) override {
    if (!value) return Error::invalid_argument;
    *value = {};
    if (storage_failure) return Error::storage_failure;
    if (!checkpoint.size) return Error::not_found;
    return decode_time_checkpoint(checkpoint, value);
  }
  Error store_time_checkpoint(TimeCheckpoint value) override {
    RecordBuffer encoded;
    const auto error = encode_time_checkpoint(value, &encoded);
    if (error != Error::none) return error;
    if (storage_failure) return Error::storage_failure;
    checkpoint = encoded;
    return Error::none;
  }
  Error erase_time_checkpoint() override {
    if (storage_failure) return Error::storage_failure;
    if (!checkpoint.size) return Error::not_found;
    clear_record(&checkpoint);
    return Error::none;
  }

 private:
  Slot* find(DeviceKey key) {
    for (auto& slot : slots) if (std::strcmp(slot.key.data(), key.value) == 0) return &slot;
    return nullptr;
  }
  Slot* acquire(DeviceKey key) {
    if (auto* slot = find(key)) return slot;
    for (auto& slot : slots) {
      if (!slot.key[0]) {
        std::memcpy(slot.key.data(), key.value, std::strlen(key.value) + 1U);
        return &slot;
      }
    }
    return nullptr;
  }
  Error erase(DeviceKey key, bool scheme) {
    if (!valid_device_key(key)) return Error::invalid_argument;
    if (storage_failure) return Error::storage_failure;
    auto* slot = find(key);
    if (!slot) return Error::not_found;
    auto& record = scheme ? slot->scheme : slot->operator_id;
    if (!record.size) return Error::not_found;
    clear_record(&record);
    if (!slot->operator_id.size && !slot->scheme.size) slot->key.fill('\0');
    return Error::none;
  }
};

constexpr DeviceKey first{"test_A-1"}, second{"test_B-2"};
// Synthetic UTF-8, including 2-, 3-, and 4-byte code points; never logged.
constexpr char synthetic[] = "test-\xc2\xa2\xe4\xb8\xad\xf0\x9f\x98\x80";
constexpr OperatorIdView identity{synthetic, sizeof(synthetic) - 1U};

bool equal(const RecordBuffer& a, const RecordBuffer& b) {
  return a.size == b.size && a.bytes == b.bytes;
}
void cleared(const RecordBuffer& record) {
  check(record.size == 0);
  for (auto byte : record.bytes) check(byte == 0);
}
void reseal(RecordBuffer& record) {
  check(record.size >= 8U && record.size <= record.bytes.size());
  const auto crc = wfrac_protocol::crc16_ccitt(record.bytes.data(), record.size - 2U);
  record.bytes[record.size - 2U] = static_cast<uint8_t>(crc);
  record.bytes[record.size - 1U] = static_cast<uint8_t>(crc >> 8U);
}
void operator_equals(IPersistence& store, DeviceKey key, OperatorIdView expected) {
  std::array<char, max_operator_id_size + 1U> out{};
  std::size_t size = 999;
  expect(store.load_operator_id(key, out.data(), out.size(), &size), Error::none);
  check(size == expected.size && out[size] == '\0');
  check(std::memcmp(out.data(), expected.data, size) == 0);
}

void roundtrip_and_erase() {
  FakePersistence fake;
  IPersistence& store = fake;
  constexpr OperatorIdView other{"synthetic-B", 11U};
  expect(store.store_operator_id(first, identity), Error::none);
  expect(store.store_operator_id(second, other), Error::none);
  expect(store.store_scheme(first, Scheme::http_confirmed), Error::none);
  expect(store.store_scheme(second, Scheme::https_confirmed), Error::none);
  operator_equals(store, first, identity);
  operator_equals(store, second, other);
  Scheme scheme = Scheme::unknown;
  expect(store.load_scheme(first, &scheme), Error::none); check(scheme == Scheme::http_confirmed);
  expect(store.load_scheme(second, &scheme), Error::none); check(scheme == Scheme::https_confirmed);
  for (const auto epoch : {int64_t{5000000000LL}, std::numeric_limits<int64_t>::max()}) {
    expect(store.store_time_checkpoint({epoch}), Error::none);
    TimeCheckpoint out{-1};
    expect(store.load_time_checkpoint(&out), Error::none); check(out.unix_seconds == epoch);
  }
  // Checkpoint is historical metadata only. This test invokes no time provider,
  // quality, clock seed, or adoption API and makes no time-trust assertion.
  static_assert(std::is_same<decltype(TimeCheckpoint::unix_seconds), int64_t>::value, "epoch width");
  expect(store.erase_operator_id(first), Error::none);
  expect(store.erase_operator_id(first), Error::not_found);
  expect(store.load_scheme(first, &scheme), Error::none); check(scheme == Scheme::http_confirmed);
  operator_equals(store, second, other);
  expect(store.erase_scheme(second), Error::none);
  expect(store.erase_scheme(second), Error::not_found);
  operator_equals(store, second, other);
  expect(store.erase_scheme(first), Error::none);
  expect(store.erase_operator_id(second), Error::none);
  expect(store.erase_time_checkpoint(), Error::none);
  expect(store.erase_time_checkpoint(), Error::not_found);
}

void invalid_stores_and_keys() {
  FakePersistence fake;
  expect(fake.store_operator_id(first, identity), Error::none);
  expect(fake.store_scheme(first, Scheme::https_confirmed), Error::none);
  expect(fake.store_time_checkpoint({5000000000LL}), Error::none);
  const auto old_id = fake.slots[0].operator_id, old_scheme = fake.slots[0].scheme;
  const auto old_time = fake.checkpoint;
  std::array<char, 129> overlong{}; overlong.fill('x');
  const char nul[] = {'x', '\0', 'y'};
  const char* malformed[] = {"\x80", "\xc0\xaf", "\xc1\xbf", "\xc2", "\xc2x",
                            "\xe0\x80\x80", "\xed\xa0\x80", "\xf0\x80\x80\x80",
                            "\xf4\x90\x80\x80", "\xf5\x80\x80\x80", "\xff"};
  auto reject = [&](OperatorIdView view) {
    RecordBuffer stale = old_id;
    expect(encode_operator_id(view, &stale), Error::invalid_argument); cleared(stale);
    expect(fake.store_operator_id(first, view), Error::invalid_argument);
    check(equal(fake.slots[0].operator_id, old_id));
  };
  reject({nullptr, 1}); reject({synthetic, 0}); reject({overlong.data(), overlong.size()});
  reject({nul, sizeof(nul)});
  for (auto value : malformed) reject({value, std::strlen(value)});
  for (auto value : {Scheme::unknown, static_cast<Scheme>(99)}) {
    RecordBuffer stale = old_scheme;
    expect(encode_scheme(value, &stale), Error::invalid_argument); cleared(stale);
    expect(fake.store_scheme(first, value), Error::invalid_argument);
    check(equal(fake.slots[0].scheme, old_scheme));
  }
  for (auto epoch : {int64_t{0}, int64_t{-1}, std::numeric_limits<int64_t>::min()}) {
    RecordBuffer stale = old_time;
    expect(encode_time_checkpoint({epoch}, &stale), Error::invalid_argument); cleared(stale);
    expect(fake.store_time_checkpoint({epoch}), Error::invalid_argument);
    check(equal(fake.checkpoint, old_time));
  }
  for (auto key : {DeviceKey{nullptr}, DeviceKey{""}, DeviceKey{"1234567890123"},
                   DeviceKey{"bad.key"}, DeviceKey{"bad key"}, DeviceKey{"\xc2\xa2"}}) {
    check(!valid_device_key(key));
    expect(fake.store_operator_id(key, identity), Error::invalid_argument);
    expect(fake.store_scheme(key, Scheme::http_confirmed), Error::invalid_argument);
    expect(fake.erase_operator_id(key), Error::invalid_argument);
    expect(fake.erase_scheme(key), Error::invalid_argument);
    check(equal(fake.slots[0].operator_id, old_id) && equal(fake.slots[0].scheme, old_scheme));
  }
  check(valid_device_key({"a"})); check(valid_device_key({"Az09_-Az09_-"}));
  expect(fake.store_operator_id({"Az09_-Az09_-"}, identity), Error::none);
  operator_equals(fake, {"Az09_-Az09_-"}, identity);
  expect(fake.store_scheme({"third"}, Scheme::http_confirmed), Error::storage_failure);
  check(equal(fake.slots[0].scheme, old_scheme));
  expect(encode_operator_id(identity, nullptr), Error::invalid_argument);
  expect(encode_scheme(Scheme::http_confirmed, nullptr), Error::invalid_argument);
  expect(encode_time_checkpoint({1}, nullptr), Error::invalid_argument);
  clear_record(nullptr);
}

void output_bounds() {
  FakePersistence fake;
  std::array<char, 128> bytes{}; bytes.fill('x'); // Non-terminated opaque bytes.
  expect(fake.store_operator_id(first, {bytes.data(), bytes.size()}), Error::none);
  operator_equals(fake, first, {bytes.data(), bytes.size()});
  std::array<char, 130> out{}; out.fill('!'); std::size_t size = 999;
  expect(fake.load_operator_id(first, out.data(), 129, &size), Error::none);
  check(size == 128 && out[128] == '\0' && out[129] == '!');
  out.fill('!'); size = 999;
  expect(fake.load_operator_id(first, out.data(), 128, &size), Error::output_too_small);
  check(size == 0 && out[0] == '\0' && out[1] == '!' && out[128] == '!');
  expect(fake.load_operator_id(first, nullptr, 0, &size), Error::output_too_small); check(size == 0);
  out[0] = '!'; size = 999;
  expect(fake.load_operator_id(first, out.data(), 0, &size), Error::output_too_small);
  check(out[0] == '!' && size == 0);
  size = 999;
  expect(fake.load_operator_id(first, nullptr, 1, &size), Error::invalid_argument); check(size == 0);
  expect(fake.load_operator_id(first, out.data(), out.size(), nullptr), Error::invalid_argument);
  check(out[0] == '\0');
  expect(fake.load_scheme(first, nullptr), Error::invalid_argument);
  expect(fake.load_time_checkpoint(nullptr), Error::invalid_argument);
  const auto& record = fake.slots[0].operator_id;
  size = 999;
  expect(decode_operator_id(record, nullptr, 1, &size), Error::invalid_argument); check(size == 0);
  out[0] = '!';
  expect(decode_operator_id(record, out.data(), out.size(), nullptr), Error::invalid_argument);
  check(out[0] == '\0');
  expect(decode_scheme(record, nullptr), Error::invalid_argument);
  expect(decode_time_checkpoint(record, nullptr), Error::invalid_argument);
}

void load_failures() {
  FakePersistence fake;
  auto errors = [&](DeviceKey key, Error expected) {
    char out[129]; std::memset(out, '!', sizeof(out)); std::size_t size = 999;
    Scheme scheme = Scheme::https_confirmed; TimeCheckpoint checkpoint{999};
    expect(fake.load_operator_id(key, out, sizeof(out), &size), expected);
    check(size == 0 && out[0] == '\0' && out[1] == '!');
    expect(fake.load_scheme(key, &scheme), expected); check(scheme == Scheme::unknown);
    expect(fake.load_time_checkpoint(&checkpoint), expected); check(checkpoint.unix_seconds == 0);
  };
  errors(first, Error::not_found);
  char out[2] = {'!', '!'}; std::size_t size = 999; Scheme scheme = Scheme::https_confirmed;
  expect(fake.load_operator_id({nullptr}, out, sizeof(out), &size), Error::invalid_argument);
  check(size == 0 && out[0] == '\0');
  expect(fake.load_scheme({nullptr}, &scheme), Error::invalid_argument); check(scheme == Scheme::unknown);
  expect(fake.store_operator_id(first, identity), Error::none);
  expect(fake.store_scheme(first, Scheme::http_confirmed), Error::none);
  expect(fake.store_time_checkpoint({1}), Error::none);
  const auto id = fake.slots[0].operator_id, sch = fake.slots[0].scheme, cp = fake.checkpoint;
  fake.storage_failure = true; errors(first, Error::storage_failure);
  expect(fake.store_operator_id(first, {"other", 5}), Error::storage_failure);
  expect(fake.store_scheme(first, Scheme::https_confirmed), Error::storage_failure);
  expect(fake.store_time_checkpoint({2}), Error::storage_failure);
  expect(fake.erase_operator_id(first), Error::storage_failure);
  expect(fake.erase_scheme(first), Error::storage_failure);
  expect(fake.erase_time_checkpoint(), Error::storage_failure);
  check(equal(fake.slots[0].operator_id, id) && equal(fake.slots[0].scheme, sch) && equal(fake.checkpoint, cp));
  fake.storage_failure = false;
  for (unsigned mutation = 0; mutation < 8; ++mutation) {
    fake.slots[0].operator_id = id; fake.slots[0].scheme = sch; fake.checkpoint = cp;
    for (auto* record : {&fake.slots[0].operator_id, &fake.slots[0].scheme, &fake.checkpoint}) {
      switch (mutation) {
        case 0: record->bytes[2] = schema_version + 1U; break;
        case 1: record->bytes[0] ^= 1U; break;
        case 2: record->bytes[3] = 99; reseal(*record); break;
        case 3: record->bytes[4] ^= 1U; reseal(*record); break;
        case 4: record->bytes[record->size - 1U] ^= 1U; break;
        case 5: --record->size; break;
        case 6: record->size = 7U; break;
        case 7: record->size = max_record_size + 1U; break;
      }
    }
    errors(first, mutation == 0 ? Error::incompatible_schema : Error::corrupt_record);
  }
  fake.slots[0].operator_id = id; fake.slots[0].scheme = sch; fake.checkpoint = cp;
  for (auto value : {uint8_t{0}, uint8_t{0x80}, uint8_t{0xff}}) {
    fake.slots[0].operator_id = id; fake.slots[0].operator_id.bytes[6] = value;
    reseal(fake.slots[0].operator_id);
    fake.slots[0].scheme = sch; fake.slots[0].scheme.bytes[6] = value;
    reseal(fake.slots[0].scheme);
    fake.checkpoint = cp;
    for (unsigned i = 0; i < 8; ++i) fake.checkpoint.bytes[6 + i] = value == 0 ? 0 : 0xff;
    reseal(fake.checkpoint);
    errors(first, Error::corrupt_record);
  }
  // Valid envelope/CRC with wrong logical payload lengths, including empty ID.
  fake.slots[0].operator_id = id; fake.slots[0].scheme = sch; fake.checkpoint = cp;
  for (auto* record : {&fake.slots[0].operator_id, &fake.slots[0].scheme, &fake.checkpoint}) {
    record->size = 8; record->bytes[4] = 0; record->bytes[5] = 0; reseal(*record);
  }
  errors(first, Error::corrupt_record);
}
}  // namespace

int main() {
  roundtrip_and_erase();
  invalid_stores_and_keys();
  output_bounds();
  load_failures();
  std::cout << "PASS: portable persistence host tests\n";
}
