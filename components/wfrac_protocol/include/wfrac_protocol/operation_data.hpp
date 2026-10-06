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
  canonical_ambient_lookup,
  exact_current_run_energy,
  inferred_diagnostic_candidate,
  inferred_extrapolated_candidate,
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

enum class OperationValueError { none, invalid_argument, unsupported_segment, out_of_established_range };

// WF-RAC network 0x80: OP1 0x10 outdoor / 0x20 indoor, OP2 lookup index.
// Not controller room state or a direct CNS formula; OP3 is preserved, not used.
struct AmbientTemperature {
  OperationSegment raw{};
  OperationSemanticKind kind = OperationSemanticKind::unknown;
  OperationScope scope = OperationScope::unknown;
  int16_t deci_celsius = 0;
};

// 0x94 / OP1 0x10: current-run, per-indoor counter, NOT lifetime energy.
// No accumulation, reset-fault inference, or cross-unit aggregation here.
struct CurrentRunEnergy {
  OperationSegment raw{};
  uint16_t quarter_kwh = 0;
  constexpr uint32_t watt_hours() const { return static_cast<uint32_t>(quarter_kwh) * 250U; }
};

// Non-null outputs reset at entry and retain all raw bytes even on unsupported
// segments. Physical metadata is valid ONLY when the return value is none.
// Allocation-free read-side decoders; no queries, writes, or consumer mutation.
OperationValueError decode_ambient_temperature(const OperationSegment& raw, AmbientTemperature* output);
OperationValueError decode_current_run_energy(const OperationSegment& raw, CurrentRunEnergy* output);

// INFERRED outdoor diagnostic candidates, NOT calibrated physical truth.
// Software arithmetic does not establish local hardware calibration/presence.
// OP3 is retained but has no admitted conversion role. No consumer mutation.
struct InferredCompressorFrequency {
  OperationSegment raw{};
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  uint16_t deci_hz = 0;
};
struct InferredOperatingCurrent {
  OperationSegment raw{};
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  uint16_t amperes_numerator = 0;
  static constexpr uint16_t amperes_denominator = 51;
};
struct InferredDischargeTemperature {
  OperationSegment raw{};
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  uint16_t half_degrees_celsius = 0;
};

// INFERRED thermistor-curve candidates for per-indoor THI-R1 (0x81) and
// THI-R3 (0x87), not calibrated or CONFIRMED_LOCAL temperatures. raw OP2=0
// has no value; raw OP2>170 is explicitly extrapolated. Output uses integer
// deci-Celsius rounded to nearest 0.1 C, with halfway cases away from zero.
struct InferredIndoorCoilTemperature {
  OperationSegment raw{};
  OperationSemanticKind kind = OperationSemanticKind::unknown;
  OperationScope scope = OperationScope::unknown;
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  int16_t deci_celsius = 0;
};

// 0x13 exposes only the exact WF-RAC network OP2 byte as an EEV-related
// pulse/value. This is not a percentage, calibrated valve opening, or the
// direct CNS 16-bit counter; OP1/OP3 are preserved without inferred meaning.
struct EevNetworkValue {
  OperationSegment raw{};
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  uint8_t network_value = 0;
};

// 0x82 exposes only the exact WF-RAC network OP2 byte for THO-R1.
// Its physical unit remains UNKNOWN; OP1/OP3 are preserved without inferred meaning.
struct OutdoorCoilRawValue {
  OperationSegment raw{};
  OperationSemanticKind kind = OperationSemanticKind::unknown;
  OperationScope scope = OperationScope::unknown;
  OperationValueQuality value_quality = OperationValueQuality::unknown_semantic;
  uint8_t network_value = 0;
};

// Non-null outputs reset and preserve raw even on failure (including aliasing).
// Candidate values/quality are usable ONLY on none; zero alone is not validity.
// 0x11 accepts only OP1 0x10..0x1f; 0x90/0x85 accept only OP1 0x10.
// 0x85 OP2 < 0x12 is out_of_established_range: no clamp/fabricated value.
OperationValueError decode_inferred_compressor_frequency(const OperationSegment& raw, InferredCompressorFrequency* output);
OperationValueError decode_inferred_operating_current(const OperationSegment& raw, InferredOperatingCurrent* output);
OperationValueError decode_inferred_discharge_temperature(const OperationSegment& raw, InferredDischargeTemperature* output);
// OP1 and OP3 are preserved but do not gate or affect this candidate curve.
// Non-null output resets at entry and preserves raw input on every failure.
OperationValueError decode_inferred_indoor_coil_temperature(const OperationSegment& raw,
                                                           InferredIndoorCoilTemperature* output);
// Non-null output resets and preserves raw on failure (including aliasing).
// network_value is meaningful only on none and equals raw OP2 exactly.
OperationValueError decode_eev_network_value(const OperationSegment& raw, EevNetworkValue* output);
// OP2=0 remains an ordinary raw byte; this decoder assigns no physical meaning.
OperationValueError decode_outdoor_coil_raw_value(const OperationSegment& raw, OutdoorCoilRawValue* output);

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
