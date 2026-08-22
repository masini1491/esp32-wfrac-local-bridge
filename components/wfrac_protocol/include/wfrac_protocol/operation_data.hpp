#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_protocol/aircon_frame.hpp"
#include "wfrac_protocol/aircon_state.hpp"

namespace wfrac_protocol {

struct OperationSegment {
  uint8_t code = 0;
  uint8_t op1 = 0;
  uint8_t op2 = 0;
  uint8_t op3 = 0;
};

enum class OperationSemanticKind {
  unknown,
  outdoor_ambient,
  indoor_ambient,
  current_run_energy,
  compressor_frequency,
  operating_current,
  discharge_hot_gas_temperature,
  eev_raw,
  thi_r1,
  thi_r3,
  tho_r1,
  tdsh_raw,
  protection_diagnostic_unknown,
  run_hours_candidate,
  fan_speed_candidate,
  unknown_semantic_0d,
  unknown_semantic_21,
  unknown_semantic_32,
  unknown_semantic_34,
  unknown_semantic_35,
  home_leave_extension,
};

enum class OperationScope { unknown, per_indoor, shared_outdoor };

enum class OperationValueQuality {
  raw_only,
  canonical_ambient_raw_pending_lookup,
  bounded_derived_available_later,
  no_usable_value_established,
  unknown_semantic,
  special_extension,
};

struct OperationData {
  OperationSegment raw{};
  OperationSemanticKind kind = OperationSemanticKind::unknown;
  OperationScope scope = OperationScope::unknown;
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  bool code_known = false;
  bool unit_relation_known = false;
};

bool operation_segment_at(const AirconBlockView& block, std::size_t index, OperationSegment* output);
OperationData decode_operation_segment(const OperationSegment& raw);

enum class OperationQueryError {
  none,
  invalid_code,
  invalid_argument,
  output_too_small,
  batch_size_out_of_range,
};

OperationSegment make_operation_query_segment(uint8_t code);
bool is_known_safe_operation_query_code(uint8_t code);
OperationQueryError build_safe_operation_query(uint8_t code, MutableByteView output, std::size_t* encoded_size);
OperationQueryError build_safe_operation_query_batch(const uint8_t* codes, std::size_t count, MutableByteView output,
                                                     std::size_t* encoded_size);

}  // namespace wfrac_protocol
