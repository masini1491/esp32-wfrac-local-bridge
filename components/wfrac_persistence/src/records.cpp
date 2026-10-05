#include "wfrac_persistence/records.hpp"

#include <cstring>
#include <limits>

#include "wfrac_protocol/crc16.hpp"

namespace wfrac_persistence {
namespace {
enum class Kind : uint8_t { operator_id = 1, scheme = 2, checkpoint = 3 };

bool valid_operator_id(OperatorIdView value) {
  if (value.data == nullptr || value.size == 0 || value.size > max_operator_id_size) return false;
  for (std::size_t i = 0; i < value.size;) {
    const auto first = static_cast<uint8_t>(value.data[i++]);
    if (first == 0) return false;
    if (first < 0x80U) continue;
    unsigned count;
    uint32_t codepoint, minimum;
    if (first >= 0xc2U && first <= 0xdfU) { count = 1; codepoint = first & 0x1fU; minimum = 0x80U; }
    else if (first >= 0xe0U && first <= 0xefU) { count = 2; codepoint = first & 0x0fU; minimum = 0x800U; }
    else if (first >= 0xf0U && first <= 0xf4U) { count = 3; codepoint = first & 0x07U; minimum = 0x10000U; }
    else return false;
    if (count > value.size - i) return false;
    for (unsigned n = 0; n < count; ++n) {
      const auto next = static_cast<uint8_t>(value.data[i++]);
      if ((next & 0xc0U) != 0x80U) return false;
      codepoint = (codepoint << 6U) | (next & 0x3fU);
    }
    if (codepoint < minimum || codepoint > 0x10ffffU || (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
      return false;
    }
  }
  return true;
}

void header(Kind kind, std::size_t payload_size, RecordBuffer* record) {
  record->bytes[0] = 'W'; record->bytes[1] = 'P';
  record->bytes[2] = schema_version; record->bytes[3] = static_cast<uint8_t>(kind);
  record->bytes[4] = static_cast<uint8_t>(payload_size);
  record->bytes[5] = static_cast<uint8_t>(payload_size >> 8U);
  record->size = 6U + payload_size + 2U;
}

void seal(RecordBuffer* record) {
  const auto crc = wfrac_protocol::crc16_ccitt(record->bytes.data(), record->size - 2U);
  record->bytes[record->size - 2U] = static_cast<uint8_t>(crc);
  record->bytes[record->size - 1U] = static_cast<uint8_t>(crc >> 8U);
}

PersistenceError validate(const RecordBuffer& record, Kind kind, std::size_t* payload_size) {
  if (record.size < 8U || record.size > record.bytes.size() || record.bytes[0] != 'W' || record.bytes[1] != 'P') {
    return PersistenceError::corrupt_record;
  }
  // Do not interpret a future schema using the current layout/CRC algorithm.
  if (record.bytes[2] != schema_version) return PersistenceError::incompatible_schema;
  const std::size_t length = record.bytes[4] | (static_cast<std::size_t>(record.bytes[5]) << 8U);
  if (record.bytes[3] != static_cast<uint8_t>(kind) || length != record.size - 8U) {
    return PersistenceError::corrupt_record;
  }
  const auto crc = wfrac_protocol::crc16_ccitt(record.bytes.data(), record.size - 2U);
  const auto stored = static_cast<uint16_t>(record.bytes[record.size - 2U]) |
                      (static_cast<uint16_t>(record.bytes[record.size - 1U]) << 8U);
  if (crc != stored) return PersistenceError::corrupt_record;
  *payload_size = length;
  return PersistenceError::none;
}
}  // namespace

bool valid_device_key(DeviceKey key) {
  if (key.value == nullptr || key.value[0] == '\0') return false;
  for (std::size_t i = 0; i <= max_device_key_size; ++i) {
    const char c = key.value[i];
    if (c == '\0') return i != 0;
    if (i == max_device_key_size || !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  }
  return false;
}

void clear_record(RecordBuffer* record) {
  if (record == nullptr) return;
  volatile uint8_t* bytes = record->bytes.data();
  for (std::size_t i = 0; i < record->bytes.size(); ++i) bytes[i] = 0;
  record->size = 0;
}

PersistenceError encode_operator_id(OperatorIdView value, RecordBuffer* record) {
  if (record == nullptr) return PersistenceError::invalid_argument;
  clear_record(record);
  if (!valid_operator_id(value)) return PersistenceError::invalid_argument;
  header(Kind::operator_id, value.size, record);
  std::memcpy(record->bytes.data() + 6U, value.data, value.size);
  seal(record);
  return PersistenceError::none;
}

PersistenceError decode_operator_id(const RecordBuffer& record, char* output, std::size_t capacity,
                                    std::size_t* size) {
  if (size != nullptr) *size = 0;
  if (output != nullptr && capacity != 0) output[0] = '\0';
  if (size == nullptr || (output == nullptr && capacity != 0)) return PersistenceError::invalid_argument;
  std::size_t length = 0;
  const auto error = validate(record, Kind::operator_id, &length);
  if (error != PersistenceError::none) return error;
  const auto* data = reinterpret_cast<const char*>(record.bytes.data() + 6U);
  if (!valid_operator_id({data, length})) return PersistenceError::corrupt_record;
  if (capacity <= length) return PersistenceError::output_too_small;
  std::memcpy(output, data, length);
  output[length] = '\0'; *size = length;
  return PersistenceError::none;
}

PersistenceError encode_scheme(wfrac_transport::SchemeKnowledge value, RecordBuffer* record) {
  if (record == nullptr) return PersistenceError::invalid_argument;
  clear_record(record);
  using wfrac_transport::SchemeKnowledge;
  if (value != SchemeKnowledge::http_confirmed && value != SchemeKnowledge::https_confirmed) {
    return PersistenceError::invalid_argument;
  }
  header(Kind::scheme, 1U, record);
  record->bytes[6] = value == SchemeKnowledge::http_confirmed ? 1U : 2U;
  seal(record);
  return PersistenceError::none;
}

PersistenceError decode_scheme(const RecordBuffer& record, wfrac_transport::SchemeKnowledge* value) {
  using wfrac_transport::SchemeKnowledge;
  if (value == nullptr) return PersistenceError::invalid_argument;
  *value = SchemeKnowledge::unknown;
  std::size_t length = 0;
  const auto error = validate(record, Kind::scheme, &length);
  if (error != PersistenceError::none) return error;
  if (length != 1U || (record.bytes[6] != 1U && record.bytes[6] != 2U)) return PersistenceError::corrupt_record;
  *value = record.bytes[6] == 1U ? SchemeKnowledge::http_confirmed : SchemeKnowledge::https_confirmed;
  return PersistenceError::none;
}

PersistenceError encode_time_checkpoint(TimeCheckpoint value, RecordBuffer* record) {
  if (record == nullptr) return PersistenceError::invalid_argument;
  clear_record(record);
  if (value.unix_seconds <= 0) return PersistenceError::invalid_argument;
  header(Kind::checkpoint, 8U, record);
  auto seconds = static_cast<uint64_t>(value.unix_seconds);
  for (unsigned i = 0; i < 8U; ++i) { record->bytes[6U + i] = static_cast<uint8_t>(seconds); seconds >>= 8U; }
  seal(record);
  return PersistenceError::none;
}

PersistenceError decode_time_checkpoint(const RecordBuffer& record, TimeCheckpoint* value) {
  if (value == nullptr) return PersistenceError::invalid_argument;
  *value = {};
  std::size_t length = 0;
  const auto error = validate(record, Kind::checkpoint, &length);
  if (error != PersistenceError::none) return error;
  if (length != 8U) return PersistenceError::corrupt_record;
  uint64_t seconds = 0;
  for (unsigned i = 0; i < 8U; ++i) seconds |= static_cast<uint64_t>(record.bytes[6U + i]) << (8U * i);
  if (seconds == 0 || seconds > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    return PersistenceError::corrupt_record;
  }
  value->unix_seconds = static_cast<int64_t>(seconds);
  return PersistenceError::none;
}

}  // namespace wfrac_persistence
