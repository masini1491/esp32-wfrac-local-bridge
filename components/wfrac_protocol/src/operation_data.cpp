#include "wfrac_protocol/operation_data.hpp"

#include <cmath>

#include "wfrac_protocol/aircon_block.hpp"
#include "ambient_tables.hpp"

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
                            OperationValueQuality::canonical_ambient_lookup);
      }
      if (raw.op1 == 0x20U) {
        return known_result(raw, OperationSemanticKind::indoor_ambient, OperationScope::per_indoor,
                            OperationValueQuality::canonical_ambient_lookup);
      }
      break;
    case 0x94U: {
      if (raw.op1 != 0x10U) break;
      OperationData result = known_result(raw, OperationSemanticKind::current_run_energy, OperationScope::per_indoor,
                                          OperationValueQuality::exact_current_run_energy);
      result.unit_relation_known = true;
      return result;
    }
    case 0x11U:
      return known_result(raw, OperationSemanticKind::compressor_frequency, OperationScope::shared_outdoor,
                          raw.op1 >= 0x10U && raw.op1 <= 0x1fU
                              ? OperationValueQuality::inferred_diagnostic_candidate : OperationValueQuality::raw_only);
    case 0x90U:
      return known_result(raw, OperationSemanticKind::operating_current, OperationScope::shared_outdoor,
                          raw.op1 == 0x10U
                              ? OperationValueQuality::inferred_diagnostic_candidate : OperationValueQuality::raw_only);
    case 0x85U:
      return known_result(raw, OperationSemanticKind::discharge_hot_gas_temperature, OperationScope::shared_outdoor,
                          raw.op1 == 0x10U && raw.op2 >= 0x12U
                              ? OperationValueQuality::inferred_diagnostic_candidate
                              : OperationValueQuality::bounded_derived_available_later);
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

OperationValueError decode_ambient_temperature(const OperationSegment& raw, AmbientTemperature* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  const auto data = decode_operation_segment(saved);
  if (data.kind != OperationSemanticKind::outdoor_ambient && data.kind != OperationSemanticKind::indoor_ambient) {
    return OperationValueError::unsupported_segment;
  }
  output->kind = data.kind;
  output->scope = data.scope;
  output->deci_celsius = data.kind == OperationSemanticKind::outdoor_ambient
      ? outdoor_deci_celsius[saved.op2] : indoor_deci_celsius[saved.op2];
  return OperationValueError::none;
}

OperationValueError decode_current_run_energy(const OperationSegment& raw, CurrentRunEnergy* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  if (decode_operation_segment(saved).kind != OperationSemanticKind::current_run_energy) {
    return OperationValueError::unsupported_segment;
  }
  output->quarter_kwh = static_cast<uint16_t>(static_cast<uint16_t>(saved.op3) << 8U) | saved.op2;
  return OperationValueError::none;
}

OperationValueError decode_inferred_compressor_frequency(const OperationSegment& raw, InferredCompressorFrequency* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  const auto data = decode_operation_segment(saved);
  if (data.kind != OperationSemanticKind::compressor_frequency ||
      data.value_quality != OperationValueQuality::inferred_diagnostic_candidate) {
    return OperationValueError::unsupported_segment;
  }
  output->deci_hz = static_cast<uint16_t>((static_cast<unsigned>(saved.op1) - 0x10U) * 256U + saved.op2);
  output->value_quality = data.value_quality;
  return OperationValueError::none;
}

OperationValueError decode_inferred_operating_current(const OperationSegment& raw, InferredOperatingCurrent* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  const auto data = decode_operation_segment(saved);
  if (data.kind != OperationSemanticKind::operating_current ||
      data.value_quality != OperationValueQuality::inferred_diagnostic_candidate) {
    return OperationValueError::unsupported_segment;
  }
  output->amperes_numerator = static_cast<uint16_t>(static_cast<unsigned>(saved.op2) * 14U);
  output->value_quality = data.value_quality;
  return OperationValueError::none;
}

OperationValueError decode_inferred_discharge_temperature(const OperationSegment& raw, InferredDischargeTemperature* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  if (saved.code != 0x85U || saved.op1 != 0x10U) return OperationValueError::unsupported_segment;
  if (saved.op2 < 0x12U) return OperationValueError::out_of_established_range;
  output->half_degrees_celsius = static_cast<uint16_t>(saved.op2 + 64U);
  output->value_quality = decode_operation_segment(saved).value_quality;
  return OperationValueError::none;
}

OperationValueError decode_inferred_indoor_coil_temperature(const OperationSegment& raw,
                                                            InferredIndoorCoilTemperature* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  const auto data = decode_operation_segment(saved);
  if (data.kind != OperationSemanticKind::thi_r1 && data.kind != OperationSemanticKind::thi_r3) {
    return OperationValueError::unsupported_segment;
  }
  if (saved.op2 == 0U) return OperationValueError::out_of_established_range;

  constexpr double kR25Ohms = 5200.0;
  constexpr double kBetaKelvin = 3900.0;
  constexpr double kReferenceKelvin = 298.15;
  constexpr double kGain = 367.0;
  constexpr double kSeriesResistanceOhms = 1912.0;
  const double raw_value = static_cast<double>(saved.op2);
  const double thermistor_ohms = kSeriesResistanceOhms * (kGain - raw_value) / raw_value;
  const double inverse_kelvin = 1.0 / kReferenceKelvin + std::log(thermistor_ohms / kR25Ohms) / kBetaKelvin;
  if (!(inverse_kelvin > 0.0)) return OperationValueError::out_of_established_range;
  const double celsius = 1.0 / inverse_kelvin - 273.15;
  const long deci_celsius = std::lround(celsius * 10.0);
  if (deci_celsius < -32768L || deci_celsius > 32767L) return OperationValueError::out_of_established_range;

  output->kind = data.kind;
  output->scope = data.scope;
  output->value_quality = saved.op2 <= 170U ? OperationValueQuality::inferred_diagnostic_candidate
                                            : OperationValueQuality::inferred_extrapolated_candidate;
  output->deci_celsius = static_cast<int16_t>(deci_celsius);
  return OperationValueError::none;
}

OperationValueError decode_eev_network_value(const OperationSegment& raw, EevNetworkValue* output) {
  if (output == nullptr) return OperationValueError::invalid_argument;
  const auto saved = raw;
  *output = {};
  output->raw = saved;
  const auto data = decode_operation_segment(saved);
  if (data.kind != OperationSemanticKind::eev_raw) return OperationValueError::unsupported_segment;
  output->network_value = saved.op2;
  output->value_quality = data.value_quality;
  return OperationValueError::none;
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
  if (encoded_size != nullptr) *encoded_size = 0U;
  if (!is_known_safe_operation_query_code(code)) return OperationQueryError::invalid_code;
  const OperationSegment segment = make_operation_query_segment(code);
  const CommandState command = make_safe_command_state();
  return map_block_error(serialize_aircon_block(command, &segment, 1U, output, encoded_size));
}

OperationQueryError build_safe_operation_query_batch(const uint8_t* codes, std::size_t count, MutableByteView output,
                                                     std::size_t* encoded_size) {
  if (encoded_size != nullptr) *encoded_size = 0U;
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
