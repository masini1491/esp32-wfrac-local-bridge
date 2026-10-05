#pragma once

#include <array>

#include "wfrac_persistence/persistence.hpp"

namespace wfrac_persistence {

// Typed record codec shared by backends, not an arbitrary key/value API.
// v1: WP magic(2), version(1), kind(1), payload length LE16, payload, CRC16 LE.
// CRC detects accidental corruption; it provides NO confidentiality/authenticity.
// Codec input, output and RecordBuffer storage must not overlap. Callers own the
// encoded/decoded sensitive bytes and their lifetime; clear_record wipes scratch.
constexpr std::size_t max_record_size = 6U + max_operator_id_size + 2U;
struct RecordBuffer {
  std::array<uint8_t, max_record_size> bytes{};
  std::size_t size = 0;
};
void clear_record(RecordBuffer* record);
PersistenceError encode_operator_id(OperatorIdView, RecordBuffer*);
PersistenceError decode_operator_id(const RecordBuffer&, char*, std::size_t, std::size_t*);
PersistenceError encode_scheme(wfrac_transport::SchemeKnowledge, RecordBuffer*);
PersistenceError decode_scheme(const RecordBuffer&, wfrac_transport::SchemeKnowledge*);
PersistenceError encode_time_checkpoint(TimeCheckpoint, RecordBuffer*);
PersistenceError decode_time_checkpoint(const RecordBuffer&, TimeCheckpoint*);

}  // namespace wfrac_persistence
