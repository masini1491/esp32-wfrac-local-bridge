#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_transport/scheme_policy.hpp"

namespace wfrac_persistence {

constexpr std::size_t max_operator_id_size = 128U;  // Project storage bound, not a firmware identity limit.
constexpr std::size_t max_device_key_size = 12U;
constexpr uint8_t schema_version = 1U;

enum class PersistenceError {
  none, invalid_argument, not_found, incompatible_schema, corrupt_record, output_too_small, storage_failure
};

// Caller-owned non-owning NUL-terminated token, 1..12 ASCII [A-Za-z0-9_-].
// Caller supplies a unique opaque stable storage key, NOT a secret payload;
// this boundary does not choose MAC/Device ID/Aircon ID identity or hash them.
struct DeviceKey { const char* value = nullptr; };
bool valid_device_key(DeviceKey key);

// Bounded UTF-8 bytes, non-empty, no embedded NUL; a terminator is not required.
// Sensitive identity is neither generated nor interpreted here. Never log it.
struct OperatorIdView { const char* data = nullptr; std::size_t size = 0; };

// Historical checkpoint only: NOT TimeSample/ITimeProvider and contains no
// trusted-quality assertion, elapsed-time estimate, automatic seed or adoption.
struct TimeCheckpoint { int64_t unix_seconds = 0; };

// Synchronous typed storage, no unrestricted public key/value interface.
// Loads reset non-null metadata on every failure. Operator output is UTF-8 plus
// NUL (size excludes NUL), never truncated; valid output[0] is cleared on failure.
// Stores validate before backend mutation. Scheme unknown is invalid: use erase.
// Erase returns not_found for an absent record. Errors do not imply rollback;
// especially storage/commit failure can leave backend outcome unknown.
// Caller owns synchronization and adoption; this interface mutates no consumer.
// Key/value input storage and load output/metadata storage must not overlap.
class IPersistence {
 public:
  virtual ~IPersistence() = default;
  virtual PersistenceError load_operator_id(DeviceKey, char*, std::size_t, std::size_t*) = 0;
  virtual PersistenceError store_operator_id(DeviceKey, OperatorIdView) = 0;
  virtual PersistenceError erase_operator_id(DeviceKey) = 0;
  virtual PersistenceError load_scheme(DeviceKey, wfrac_transport::SchemeKnowledge*) = 0;
  virtual PersistenceError store_scheme(DeviceKey, wfrac_transport::SchemeKnowledge) = 0;
  virtual PersistenceError erase_scheme(DeviceKey) = 0;
  virtual PersistenceError load_time_checkpoint(TimeCheckpoint*) = 0;
  virtual PersistenceError store_time_checkpoint(TimeCheckpoint) = 0;
  virtual PersistenceError erase_time_checkpoint() = 0;
};

}  // namespace wfrac_persistence
