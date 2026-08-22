#include "wfrac_protocol/operation_data.hpp"

#include "wfrac_protocol/aircon_block.hpp"

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

OperationSegment make_operation_query_segment(uint8_t code) { return {code, 0xFFU, 0xFFU, 0xFFU}; }

bool is_known_safe_operation_query_code(uint8_t code) {
  switch (code) {
    case 0x11U:
    case 0x90U:
    case 0x85U:
    case 0x13U:
    case 0x81U:
    case 0x87U:
    case 0x82U:
    case 0xB1U:
    case 0x7CU:
    case 0x1EU:
    case 0x1FU:
    case 0x0DU:
    case 0x21U:
    case 0x32U:
    case 0x34U:
    case 0x35U:
      return true;
    default:
      return false;
  }
}

namespace {

OperationQueryError map_block_error(BlockEncodeError error) {
  switch (error) {
    case BlockEncodeError::none: return OperationQueryError::none;
    case BlockEncodeError::output_too_small: return OperationQueryError::output_too_small;
    case BlockEncodeError::invalid_argument:
    case BlockEncodeError::invalid_segment_view:
    case BlockEncodeError::segment_count_overflow:
      return OperationQueryError::invalid_argument;
  }
  return OperationQueryError::invalid_argument;
}

}  // namespace

OperationQueryError build_safe_operation_query(uint8_t code, MutableByteView output, std::size_t* encoded_size) {
  if (!is_known_safe_operation_query_code(code)) return OperationQueryError::invalid_code;
  const OperationSegment segment = make_operation_query_segment(code);
  const CommandState command = make_safe_command_state();
  return map_block_error(serialize_aircon_block(command, &segment, 1U, output, encoded_size));
}

OperationQueryError build_safe_operation_query_batch(const uint8_t* codes, std::size_t count, MutableByteView output,
                                                     std::size_t* encoded_size) {
  if (count == 0U || count > 3U) return OperationQueryError::batch_size_out_of_range;
  if (codes == nullptr) return OperationQueryError::invalid_argument;
  OperationSegment segments[3]{};
  for (std::size_t index = 0; index < count; ++index) {
    if (!is_known_safe_operation_query_code(codes[index])) return OperationQueryError::invalid_code;
    segments[index] = make_operation_query_segment(codes[index]);
  }
  const CommandState command = make_safe_command_state();
  return map_block_error(serialize_aircon_block(command, segments, count, output, encoded_size));
}

}  // namespace wfrac_protocol
