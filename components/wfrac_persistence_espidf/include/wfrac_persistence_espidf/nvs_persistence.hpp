#pragma once

#include "wfrac_persistence/persistence.hpp"
#include "wfrac_persistence/records.hpp"

namespace wfrac_persistence_espidf {

// Compile-only in the current Stage. Construction performs NO NVS operations.
// Future caller must explicitly initialize the default NVS partition, authorize
// runtime/secret storage, and serialize calls. No auto init, erase or migration.
// Default partition blobs are NOT an encrypted-storage/confidentiality claim.
// Namespace wd_<opaque key> (<=15 bytes): operator/scheme; wf_time: checkpoint.
// Version lives inside each bounded blob. Keys never contain stored secret data.
class NvsPersistence final : public wfrac_persistence::IPersistence {
 public:
  wfrac_persistence::PersistenceError load_operator_id(
      wfrac_persistence::DeviceKey, char*, std::size_t, std::size_t*) override;
  wfrac_persistence::PersistenceError store_operator_id(
      wfrac_persistence::DeviceKey, wfrac_persistence::OperatorIdView) override;
  wfrac_persistence::PersistenceError erase_operator_id(wfrac_persistence::DeviceKey) override;
  wfrac_persistence::PersistenceError load_scheme(
      wfrac_persistence::DeviceKey, wfrac_transport::SchemeKnowledge*) override;
  wfrac_persistence::PersistenceError store_scheme(
      wfrac_persistence::DeviceKey, wfrac_transport::SchemeKnowledge) override;
  wfrac_persistence::PersistenceError erase_scheme(wfrac_persistence::DeviceKey) override;
  wfrac_persistence::PersistenceError load_time_checkpoint(wfrac_persistence::TimeCheckpoint*) override;
  wfrac_persistence::PersistenceError store_time_checkpoint(wfrac_persistence::TimeCheckpoint) override;
  wfrac_persistence::PersistenceError erase_time_checkpoint() override;

 private:
  wfrac_persistence::PersistenceError read_record(const char*, const char*, wfrac_persistence::RecordBuffer*);
  wfrac_persistence::PersistenceError write_record(const char*, const char*, const wfrac_persistence::RecordBuffer&);
  wfrac_persistence::PersistenceError erase_record(const char*, const char*);
};

}  // namespace wfrac_persistence_espidf
