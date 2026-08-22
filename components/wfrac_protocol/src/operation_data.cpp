#include "wfrac_protocol/operation_data.hpp"

namespace wfrac_protocol {
namespace {

OperationData known_result(const OperationSegment& raw, OperationSemanticKind kind, OperationScope scope,
                          OperationValueQuality quality) {
  OperationData result{};
  result.raw = raw;
  result.kind = kind;
  result.scope = scope;
  result.value_quality = quality;
  result.code_known = true;
  return result;
}

}  // namespace

bool operation_segment_at(const AirconBlockView& block, std::size_t index, OperationSegment* output) {
  if (output == nullptr || index >= block.segment_count || block.segments.data == nullptr) return false;
  const std::size_t offset = index * kAirconSegmentBytes;
  if (offset > block.segments.size || block.segments.size - offset < kAirconSegmentBytes) return false;
  output->code = block.segments.data[offset];
  output->op1 = block.segments.data[offset + 1U];
  output->op2 = block.segments.data[offset + 2U];
  output->op3 = block.segments.data[offset + 3U];
  return true;
}

OperationData decode_operation_segment(const OperationSegment& raw) {
  switch (raw.code) {
    case 0x80U:
      if (raw.op1 == 0x10U) {
        return known_result(raw, OperationSemanticKind::outdoor_ambient, OperationScope::shared_outdoor,
                            OperationValueQuality::canonical_ambient_raw_pending_lookup);
      }
      if (raw.op1 == 0x20U) {
        return known_result(raw, OperationSemanticKind::indoor_ambient, OperationScope::per_indoor,
                            OperationValueQuality::canonical_ambient_raw_pending_lookup);
      }
      break;
    case 0x94U: {
      OperationData result = known_result(raw, OperationSemanticKind::current_run_energy, OperationScope::per_indoor,
                                          OperationValueQuality::bounded_derived_available_later);
      result.unit_relation_known = true;
      return result;
    }
    case 0x11U:
      return known_result(raw, OperationSemanticKind::compressor_frequency, OperationScope::shared_outdoor,
                          OperationValueQuality::raw_only);
    case 0x90U:
      return known_result(raw, OperationSemanticKind::operating_current, OperationScope::shared_outdoor,
                          OperationValueQuality::raw_only);
    case 0x85U:
      return known_result(raw, OperationSemanticKind::discharge_hot_gas_temperature, OperationScope::shared_outdoor,
                          OperationValueQuality::bounded_derived_available_later);
    case 0x13U:
      return known_result(raw, OperationSemanticKind::eev_raw, OperationScope::per_indoor,
                          OperationValueQuality::raw_only);
    case 0x81U:
      return known_result(raw, OperationSemanticKind::thi_r1, OperationScope::per_indoor,
                          OperationValueQuality::bounded_derived_available_later);
    case 0x87U:
      return known_result(raw, OperationSemanticKind::thi_r3, OperationScope::per_indoor,
                          OperationValueQuality::bounded_derived_available_later);
    case 0x82U:
      return known_result(raw, OperationSemanticKind::tho_r1, OperationScope::shared_outdoor,
                          OperationValueQuality::raw_only);
    case 0xB1U:
      return known_result(raw, OperationSemanticKind::tdsh_raw, OperationScope::unknown,
                          OperationValueQuality::raw_only);
    case 0x7CU:
      return known_result(raw, OperationSemanticKind::protection_diagnostic_unknown, OperationScope::unknown,
                          OperationValueQuality::unknown_semantic);
    case 0x1EU:
      return known_result(raw, OperationSemanticKind::run_hours_candidate, OperationScope::unknown,
                          OperationValueQuality::no_usable_value_established);
    case 0x1FU:
      return known_result(raw, OperationSemanticKind::fan_speed_candidate, OperationScope::unknown,
                          OperationValueQuality::no_usable_value_established);
    case 0x0DU:
      return known_result(raw, OperationSemanticKind::unknown_semantic_0d, OperationScope::unknown,
                          OperationValueQuality::unknown_semantic);
    case 0x21U:
      return known_result(raw, OperationSemanticKind::unknown_semantic_21, OperationScope::unknown,
                          OperationValueQuality::unknown_semantic);
    case 0x32U:
      return known_result(raw, OperationSemanticKind::unknown_semantic_32, OperationScope::unknown,
                          OperationValueQuality::unknown_semantic);
    case 0x34U:
      return known_result(raw, OperationSemanticKind::unknown_semantic_34, OperationScope::unknown,
                          OperationValueQuality::unknown_semantic);
    case 0x35U:
      return known_result(raw, OperationSemanticKind::unknown_semantic_35, OperationScope::unknown,
                          OperationValueQuality::unknown_semantic);
    case 0xF8U:
      return known_result(raw, OperationSemanticKind::home_leave_extension, OperationScope::unknown,
                          OperationValueQuality::special_extension);
    default:
      break;
  }
  OperationData result{};
  result.raw = raw;
  return result;
}

}  // namespace wfrac_protocol
