#include "wfrac_persistence_espidf/nvs_persistence.hpp"

#include <cstring>

#include "nvs.h"

namespace wfrac_persistence_espidf {
namespace {
using namespace wfrac_persistence;

struct ScopedRecord {
  RecordBuffer record{};
  ~ScopedRecord() { clear_record(&record); }
};

PersistenceError storage_error(esp_err_t error) {
  if (error == ESP_OK) return PersistenceError::none;
  if (error == ESP_ERR_NVS_NOT_FOUND) return PersistenceError::not_found;
  if (error == ESP_ERR_NVS_TYPE_MISMATCH || error == ESP_ERR_NVS_INVALID_LENGTH) {
    return PersistenceError::corrupt_record;
  }
  return PersistenceError::storage_failure;
}

bool device_namespace(DeviceKey key, char (&name)[16]) {
  if (!valid_device_key(key)) return false;
  std::memcpy(name, "wd_", 3U);
  std::memcpy(name + 3U, key.value, std::strlen(key.value) + 1U);
  return true;
}
}  // namespace

wfrac_persistence::PersistenceError NvsPersistence::read_record(
    const char* name, const char* key, wfrac_persistence::RecordBuffer* record) {
  using namespace wfrac_persistence;
  clear_record(record);
  nvs_handle_t handle{};
  auto error = nvs_open(name, NVS_READONLY, &handle);
  if (error != ESP_OK) return storage_error(error);
  std::size_t required = 0;
  error = nvs_get_blob(handle, key, nullptr, &required);
  if (error != ESP_OK) { nvs_close(handle); return storage_error(error); }
  if (required < 8U || required > record->bytes.size()) {
    nvs_close(handle); return PersistenceError::corrupt_record;
  }
  std::size_t actual = required;
  error = nvs_get_blob(handle, key, record->bytes.data(), &actual);
  nvs_close(handle);
  if (error != ESP_OK) { clear_record(record); return storage_error(error); }
  if (actual != required) { clear_record(record); return PersistenceError::corrupt_record; }
  record->size = actual;
  return PersistenceError::none;
}

wfrac_persistence::PersistenceError NvsPersistence::write_record(
    const char* name, const char* key, const wfrac_persistence::RecordBuffer& record) {
  nvs_handle_t handle{};
  auto error = nvs_open(name, NVS_READWRITE, &handle);
  if (error != ESP_OK) return storage_error(error);
  error = nvs_set_blob(handle, key, record.bytes.data(), record.size);
  if (error == ESP_OK) error = nvs_commit(handle);
  nvs_close(handle);
  return storage_error(error);  // Failure is not a rollback/durability assertion.
}

wfrac_persistence::PersistenceError NvsPersistence::erase_record(const char* name, const char* key) {
  nvs_handle_t handle{};
  // Read-only existence check avoids creating a namespace just to erase absence.
  auto error = nvs_open(name, NVS_READONLY, &handle);
  if (error != ESP_OK) return storage_error(error);
  std::size_t length = 0;
  error = nvs_get_blob(handle, key, nullptr, &length);
  nvs_close(handle);
  // A wrong-type record is still explicitly erasable, without interpreting it.
  if (error != ESP_OK && error != ESP_ERR_NVS_TYPE_MISMATCH) return storage_error(error);
  error = nvs_open(name, NVS_READWRITE, &handle);
  if (error != ESP_OK) return storage_error(error);
  error = nvs_erase_key(handle, key);
  if (error == ESP_OK) error = nvs_commit(handle);
  nvs_close(handle);
  return storage_error(error);
}

wfrac_persistence::PersistenceError NvsPersistence::load_operator_id(
    wfrac_persistence::DeviceKey key, char* output, std::size_t capacity, std::size_t* size) {
  using namespace wfrac_persistence;
  if (size != nullptr) *size = 0;
  if (output != nullptr && capacity != 0) output[0] = '\0';
  char name[16]{};
  if (size == nullptr || (output == nullptr && capacity != 0) || !device_namespace(key, name)) {
    return PersistenceError::invalid_argument;
  }
  ScopedRecord scratch;
  const auto error = read_record(name, "operator", &scratch.record);
  return error == PersistenceError::none ? decode_operator_id(scratch.record, output, capacity, size) : error;
}

wfrac_persistence::PersistenceError NvsPersistence::store_operator_id(
    wfrac_persistence::DeviceKey key, wfrac_persistence::OperatorIdView value) {
  using namespace wfrac_persistence;
  char name[16]{};
  if (!device_namespace(key, name)) return PersistenceError::invalid_argument;
  ScopedRecord scratch;
  const auto error = encode_operator_id(value, &scratch.record);
  return error == PersistenceError::none ? write_record(name, "operator", scratch.record) : error;
}

wfrac_persistence::PersistenceError NvsPersistence::erase_operator_id(wfrac_persistence::DeviceKey key) {
  char name[16]{};
  return device_namespace(key, name) ? erase_record(name, "operator") : wfrac_persistence::PersistenceError::invalid_argument;
}

wfrac_persistence::PersistenceError NvsPersistence::load_scheme(
    wfrac_persistence::DeviceKey key, wfrac_transport::SchemeKnowledge* value) {
  using namespace wfrac_persistence;
  if (value != nullptr) *value = wfrac_transport::SchemeKnowledge::unknown;
  char name[16]{};
  if (value == nullptr || !device_namespace(key, name)) return PersistenceError::invalid_argument;
  ScopedRecord scratch;
  const auto error = read_record(name, "scheme", &scratch.record);
  return error == PersistenceError::none ? decode_scheme(scratch.record, value) : error;
}

wfrac_persistence::PersistenceError NvsPersistence::store_scheme(
    wfrac_persistence::DeviceKey key, wfrac_transport::SchemeKnowledge value) {
  using namespace wfrac_persistence;
  char name[16]{};
  if (!device_namespace(key, name)) return PersistenceError::invalid_argument;
  ScopedRecord scratch;
  const auto error = encode_scheme(value, &scratch.record);
  return error == PersistenceError::none ? write_record(name, "scheme", scratch.record) : error;
}

wfrac_persistence::PersistenceError NvsPersistence::erase_scheme(wfrac_persistence::DeviceKey key) {
  char name[16]{};
  return device_namespace(key, name) ? erase_record(name, "scheme") : wfrac_persistence::PersistenceError::invalid_argument;
}

wfrac_persistence::PersistenceError NvsPersistence::load_time_checkpoint(wfrac_persistence::TimeCheckpoint* value) {
  using namespace wfrac_persistence;
  if (value == nullptr) return PersistenceError::invalid_argument;
  *value = {};
  ScopedRecord scratch;
  const auto error = read_record("wf_time", "checkpoint", &scratch.record);
  return error == PersistenceError::none ? decode_time_checkpoint(scratch.record, value) : error;
}

wfrac_persistence::PersistenceError NvsPersistence::store_time_checkpoint(wfrac_persistence::TimeCheckpoint value) {
  using namespace wfrac_persistence;
  ScopedRecord scratch;
  const auto error = encode_time_checkpoint(value, &scratch.record);
  return error == PersistenceError::none ? write_record("wf_time", "checkpoint", scratch.record) : error;
}

wfrac_persistence::PersistenceError NvsPersistence::erase_time_checkpoint() {
  return erase_record("wf_time", "checkpoint");
}

}  // namespace wfrac_persistence_espidf
