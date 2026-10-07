#include <cassert>
#include <cstdint>
#include <initializer_list>

#include "wfrac_protocol/operation_data.hpp"

namespace {
using namespace wfrac_protocol;

void same_raw(const OperationSegment& actual, const OperationSegment& expected) {
  assert(actual.code == expected.code && actual.op1 == expected.op1);
  assert(actual.op2 == expected.op2 && actual.op3 == expected.op3);
}

uint64_t fingerprint(uint64_t hash, int16_t value) {
  const auto bits = static_cast<uint16_t>(value);
  for (unsigned shift : {0U, 8U}) {
    hash ^= (bits >> shift) & 0xffU;
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

void test_inferred_outdoor_candidates() {
  // Exact arithmetic tests validate software, NOT calibration of the INF formulas
  // in pinned wf-rac-module-reference.md section 5.4 (928efd8637bc...).
  InferredCompressorFrequency frequency{};
  InferredOperatingCurrent current{};
  InferredDischargeTemperature temperature{};
  for (unsigned selector = 0; selector < 256U; ++selector) {
    const auto op1 = static_cast<uint8_t>(selector);
    const OperationSegment raw{0x11U, op1, 0xffU, 0xa5U};
    frequency.deci_hz = 999U;
    frequency.value_quality = OperationValueQuality::canonical_ambient_lookup;
    const auto error = decode_inferred_compressor_frequency(raw, &frequency);
    same_raw(frequency.raw, raw);
    if (selector >= 0x10U && selector <= 0x1fU) {
      assert(error == OperationValueError::none);
      assert(frequency.deci_hz == (selector - 0x10U) * 256U + 255U);
      assert(frequency.value_quality == OperationValueQuality::inferred_diagnostic_candidate);
    } else {
      assert(error == OperationValueError::unsupported_segment);
      assert(frequency.deci_hz == 0 && frequency.value_quality == OperationValueQuality::unknown_semantic);
    }
    if (selector != 0x10U) {
      current.amperes_numerator = 999U;
      temperature.half_degrees_celsius = 999U;
      current.value_quality = temperature.value_quality = OperationValueQuality::canonical_ambient_lookup;
      const OperationSegment cur{0x90U, op1, 0xffU, 0xa5U}, temp{0x85U, op1, 0xffU, 0xa5U};
      assert(decode_inferred_operating_current(cur, &current) == OperationValueError::unsupported_segment);
      assert(decode_inferred_discharge_temperature(temp, &temperature) == OperationValueError::unsupported_segment);
      same_raw(current.raw, cur); same_raw(temperature.raw, temp);
      assert(current.amperes_numerator == 0 && temperature.half_degrees_celsius == 0);
      assert(current.value_quality == OperationValueQuality::unknown_semantic);
      assert(temperature.value_quality == OperationValueQuality::unknown_semantic);
    }
  }
  assert(decode_inferred_compressor_frequency({0x11U, 0x10U, 0, 0}, &frequency) == OperationValueError::none);
  assert(frequency.deci_hz == 0);
  assert(decode_inferred_compressor_frequency({0x11U, 0x10U, 0xe6U, 0xffU}, &frequency) == OperationValueError::none);
  assert(frequency.deci_hz == 230U);
  assert(decode_inferred_compressor_frequency({0x11U, 0x1fU, 0xffU, 0}, &frequency) == OperationValueError::none);
  assert(frequency.deci_hz == 4095U);
  for (unsigned value = 0; value < 256U; ++value) {
    const auto byte = static_cast<uint8_t>(value);
    const OperationSegment cur{0x90U, 0x10U, byte, 0x5aU}, temp{0x85U, 0x10U, byte, 0xa5U};
    assert(decode_inferred_operating_current(cur, &current) == OperationValueError::none);
    assert(current.amperes_numerator == value * 14U && current.amperes_denominator == 51U);
    assert(current.value_quality == OperationValueQuality::inferred_diagnostic_candidate);
    same_raw(current.raw, cur);
    temperature.half_degrees_celsius = 999U;
    temperature.value_quality = OperationValueQuality::inferred_diagnostic_candidate;
    const auto error = decode_inferred_discharge_temperature(temp, &temperature);
    same_raw(temperature.raw, temp);
    if (value < 0x12U) {
      assert(error == OperationValueError::out_of_established_range);
      assert(temperature.half_degrees_celsius == 0); // no 30 C clamp (60), nor fabricated 32 C (64)
      assert(temperature.value_quality == OperationValueQuality::unknown_semantic);
      assert(decode_operation_segment(temp).value_quality != OperationValueQuality::inferred_diagnostic_candidate);
    } else {
      assert(error == OperationValueError::none && temperature.half_degrees_celsius == value + 64U);
      assert(temperature.value_quality == OperationValueQuality::inferred_diagnostic_candidate);
    }
  }
  assert(decode_inferred_operating_current({0x90U, 0x10U, 4U, 0}, &current) == OperationValueError::none);
  assert(current.amperes_numerator == 56U); // exact 56/51, NOT rounded/truncated 1 A
  assert(decode_inferred_discharge_temperature({0x85U, 0x10U, 0x12U, 0}, &temperature) == OperationValueError::none);
  assert(temperature.half_degrees_celsius == 82U);
  assert(decode_inferred_discharge_temperature({0x85U, 0x10U, 0x14U, 0}, &temperature) == OperationValueError::none);
  assert(temperature.half_degrees_celsius == 84U);
  assert(decode_inferred_discharge_temperature({0x85U, 0x10U, 0x17U, 0}, &temperature) == OperationValueError::none);
  assert(temperature.half_degrees_celsius == 87U);
  // Retained input may alias its output; reset must not corrupt it.
  assert(decode_inferred_compressor_frequency(frequency.raw, &frequency) == OperationValueError::none);
  assert(frequency.deci_hz == 4095U);
  assert(decode_inferred_operating_current(current.raw, &current) == OperationValueError::none);
  assert(current.amperes_numerator == 56U);
  assert(decode_inferred_discharge_temperature(temperature.raw, &temperature) == OperationValueError::none);
  assert(temperature.half_degrees_celsius == 87U);
  for (unsigned code = 0; code < 256U; ++code) {
    const OperationSegment raw{static_cast<uint8_t>(code), 0x10U, 0x12U, 0xffU};
    if (code != 0x11U) {
      assert(decode_inferred_compressor_frequency(raw, &frequency) == OperationValueError::unsupported_segment);
      assert(frequency.deci_hz == 0 && frequency.value_quality == OperationValueQuality::unknown_semantic);
      same_raw(frequency.raw, raw);
    }
    if (code != 0x90U) {
      assert(decode_inferred_operating_current(raw, &current) == OperationValueError::unsupported_segment);
      assert(current.amperes_numerator == 0 && current.value_quality == OperationValueQuality::unknown_semantic);
      same_raw(current.raw, raw);
    }
    if (code != 0x85U) {
      assert(decode_inferred_discharge_temperature(raw, &temperature) == OperationValueError::unsupported_segment);
      assert(temperature.half_degrees_celsius == 0 && temperature.value_quality == OperationValueQuality::unknown_semantic);
      same_raw(temperature.raw, raw);
    }
    if (code == 0x11U || code == 0x90U || code == 0x85U) {
      const auto data = decode_operation_segment(raw);
      assert(data.scope == OperationScope::shared_outdoor && !data.unit_relation_known);
      assert(data.value_quality == OperationValueQuality::inferred_diagnostic_candidate);
    }
  }
  assert(decode_inferred_compressor_frequency({}, nullptr) == OperationValueError::invalid_argument);
  assert(decode_inferred_operating_current({}, nullptr) == OperationValueError::invalid_argument);
  assert(decode_inferred_discharge_temperature({}, nullptr) == OperationValueError::invalid_argument);
}

void test_eev_network_value() {
  EevNetworkValue value{};
  const auto classified = decode_operation_segment({0x13U, 0xA7U, 102U, 0x5CU});
  assert(classified.kind == OperationSemanticKind::eev_raw);
  assert(classified.scope == OperationScope::per_indoor);
  assert(classified.value_quality == OperationValueQuality::raw_only);
  assert(classified.raw.code == 0x13U && classified.raw.op1 == 0xA7U &&
         classified.raw.op2 == 102U && classified.raw.op3 == 0x5CU);
  for (const uint8_t op2 : {uint8_t{0U}, uint8_t{57U}, uint8_t{95U}, uint8_t{102U}, uint8_t{200U}, uint8_t{255U}}) {
    const OperationSegment raw{0x13U, 0xA7U, op2, 0x5CU};
    value.network_value = 42U;
    value.value_quality = OperationValueQuality::canonical_ambient_lookup;
    assert(decode_eev_network_value(raw, &value) == OperationValueError::none);
    same_raw(value.raw, raw);
    assert(value.network_value == op2);
    assert(value.value_quality == OperationValueQuality::raw_only);
  }

  // Input may alias the output's raw view; reset must preserve the whole segment.
  value.raw = {0x13U, 0x11U, 102U, 0xE3U};
  assert(decode_eev_network_value(value.raw, &value) == OperationValueError::none);
  assert(value.network_value == 102U && value.value_quality == OperationValueQuality::raw_only);
  same_raw(value.raw, {0x13U, 0x11U, 102U, 0xE3U});

  const OperationSegment unsupported{0x12U, 0x11U, 102U, 0xE3U};
  value.network_value = 99U;
  value.value_quality = OperationValueQuality::inferred_diagnostic_candidate;
  assert(decode_eev_network_value(unsupported, &value) == OperationValueError::unsupported_segment);
  same_raw(value.raw, unsupported);
  assert(value.network_value == 0U && value.value_quality == OperationValueQuality::unknown_semantic);
  assert(decode_eev_network_value({}, nullptr) == OperationValueError::invalid_argument);
}

void test_outdoor_coil_raw_value() {
  OutdoorCoilRawValue value{};
  for (const uint8_t op2 : {uint8_t{0U}, uint8_t{63U}, uint8_t{73U}, uint8_t{255U}}) {
    const OperationSegment raw{0x82U, static_cast<uint8_t>(op2 ^ 0xA5U), op2,
                               static_cast<uint8_t>(op2 ^ 0x5AU)};
    value.network_value = 42U;
    value.kind = OperationSemanticKind::thi_r1;
    value.scope = OperationScope::per_indoor;
    value.value_quality = OperationValueQuality::canonical_ambient_lookup;
    assert(decode_outdoor_coil_raw_value(raw, &value) == OperationValueError::none);
    same_raw(value.raw, raw);
    assert(value.network_value == op2);
    assert(value.kind == OperationSemanticKind::tho_r1);
    assert(value.scope == OperationScope::shared_outdoor);
    assert(value.value_quality == OperationValueQuality::raw_only);
    const auto classified = decode_operation_segment(raw);
    assert(classified.kind == OperationSemanticKind::tho_r1);
    assert(classified.scope == OperationScope::shared_outdoor);
    assert(classified.value_quality == OperationValueQuality::raw_only);
    assert(!classified.unit_relation_known);
    InferredIndoorCoilTemperature indoor{};
    assert(decode_inferred_indoor_coil_temperature(raw, &indoor) == OperationValueError::unsupported_segment);
  }

  // OP1 is not a selector gate, OP3 is preserved, and input may alias output.
  value.raw = {0x82U, 0xE1U, 0U, 0x7BU};
  assert(decode_outdoor_coil_raw_value(value.raw, &value) == OperationValueError::none);
  assert(value.network_value == 0U && value.raw.op1 == 0xE1U && value.raw.op3 == 0x7BU);
  assert(value.kind == OperationSemanticKind::tho_r1 && value.scope == OperationScope::shared_outdoor);
  assert(value.value_quality == OperationValueQuality::raw_only);

  const OperationSegment unsupported{0x81U, 0xFFU, 73U, 0xFFU};
  value.network_value = 99U;
  value.kind = OperationSemanticKind::tho_r1;
  value.scope = OperationScope::shared_outdoor;
  value.value_quality = OperationValueQuality::inferred_diagnostic_candidate;
  assert(decode_outdoor_coil_raw_value(unsupported, &value) == OperationValueError::unsupported_segment);
  same_raw(value.raw, unsupported);
  assert(value.network_value == 0U && value.kind == OperationSemanticKind::unknown);
  assert(value.scope == OperationScope::unknown && value.value_quality == OperationValueQuality::unknown_semantic);
  assert(decode_outdoor_coil_raw_value({}, nullptr) == OperationValueError::invalid_argument);
}

void test_tdsh_raw_value() {
  TdshRawValue value{};
  for (const uint8_t op2 : {uint8_t{0U}, uint8_t{1U}, uint8_t{127U}, uint8_t{255U}}) {
    const OperationSegment raw{0xB1U, static_cast<uint8_t>(op2 ^ 0xA5U), op2,
                               static_cast<uint8_t>(op2 ^ 0x5AU)};
    value.network_value = 42U;
    value.kind = OperationSemanticKind::tho_r1;
    value.scope = OperationScope::per_indoor;
    value.value_quality = OperationValueQuality::inferred_diagnostic_candidate;
    assert(decode_tdsh_raw_value(raw, &value) == OperationValueError::none);
    same_raw(value.raw, raw);
    assert(value.network_value == op2);
    assert(value.kind == OperationSemanticKind::tdsh_raw);
    assert(value.scope == OperationScope::shared_outdoor);
    assert(value.value_quality == OperationValueQuality::raw_only);
    const auto classified = decode_operation_segment(raw);
    assert(classified.kind == OperationSemanticKind::tdsh_raw);
    assert(classified.scope == OperationScope::shared_outdoor);
    assert(classified.value_quality == OperationValueQuality::raw_only);
    assert(!classified.unit_relation_known);
  }

  // No OP1 selector is imposed, OP3 is preserved, and input may alias output.
  value.raw = {0xB1U, 0xE1U, 0U, 0x7BU};
  assert(decode_tdsh_raw_value(value.raw, &value) == OperationValueError::none);
  assert(value.network_value == 0U && value.raw.op1 == 0xE1U && value.raw.op3 == 0x7BU);
  assert(value.kind == OperationSemanticKind::tdsh_raw && value.scope == OperationScope::shared_outdoor);
  assert(value.value_quality == OperationValueQuality::raw_only);

  const OperationSegment unsupported{0x82U, 0xFFU, 73U, 0xFFU};
  value.network_value = 99U;
  value.kind = OperationSemanticKind::tdsh_raw;
  value.scope = OperationScope::shared_outdoor;
  value.value_quality = OperationValueQuality::raw_only;
  assert(decode_tdsh_raw_value(unsupported, &value) == OperationValueError::unsupported_segment);
  same_raw(value.raw, unsupported);
  assert(value.network_value == 0U && value.kind == OperationSemanticKind::unknown);
  assert(value.scope == OperationScope::unknown && value.value_quality == OperationValueQuality::unknown_semantic);
  assert(decode_tdsh_raw_value({}, nullptr) == OperationValueError::invalid_argument);
}

void test_protection_diagnostic_unknown_guards() {
  const OperationSegment protection_zero{0x7CU, 0xA5U, 0x00U, 0x5AU};
  const OperationSegment protection_nonzero{0x7CU, 0x02U, 0xD3U, 0xFEU};
  for (const OperationSegment raw : {protection_zero, protection_nonzero}) {
    const OperationData classified = decode_operation_segment(raw);
    same_raw(classified.raw, raw);
    assert(classified.kind == OperationSemanticKind::protection_diagnostic_unknown);
    assert(classified.scope == OperationScope::unknown);
    assert(classified.value_quality == OperationValueQuality::unknown_semantic);
    assert(classified.code_known && !classified.unit_relation_known);
    assert(is_known_safe_operation_query_code(raw.code));
  }

  const auto tdsh = decode_operation_segment({0xB1U, 0x13U, 0x00U, 0x12U});
  assert(tdsh.kind == OperationSemanticKind::tdsh_raw && tdsh.scope == OperationScope::shared_outdoor);
  assert(tdsh.value_quality == OperationValueQuality::raw_only);
  const auto no_run_value = decode_operation_segment({0x1EU, 0x15U, 0x00U, 0x17U});
  assert(no_run_value.kind == OperationSemanticKind::run_hours_candidate);
  assert(no_run_value.value_quality == OperationValueQuality::no_usable_value_established);
  const auto no_fan_value = decode_operation_segment({0x1FU, 0x18U, 0x00U, 0x1AU});
  assert(no_fan_value.kind == OperationSemanticKind::fan_speed_candidate);
  assert(no_fan_value.value_quality == OperationValueQuality::no_usable_value_established);
  const auto unknown = decode_operation_segment({0xFEU, 0x29U, 0x00U, 0x2BU});
  assert(unknown.kind == OperationSemanticKind::unknown && !unknown.code_known);
  assert(unknown.scope == OperationScope::unknown && unknown.value_quality == OperationValueQuality::unknown_semantic);
}

void test_response_no_value_semantics() {
  const OperationSegment responses[] = {
      {0x1EU, 0xFFU, 0xFFU, 0xFFU},
      {0x1FU, 0xFFU, 0xFFU, 0xFFU},
  };
  const OperationSemanticKind kinds[] = {
      OperationSemanticKind::run_hours_candidate,
      OperationSemanticKind::fan_speed_candidate,
  };
  for (unsigned i = 0; i < 2U; ++i) {
    const auto& raw = responses[i];
    const auto value = decode_operation_segment(raw);
    same_raw(value.raw, raw);
    assert(value.kind == kinds[i]);
    assert(value.scope == OperationScope::unknown);
    assert(value.value_quality == OperationValueQuality::no_usable_value_established);
    assert(value.code_known && !value.unit_relation_known);
    assert(is_known_safe_operation_query_code(raw.code));
  }
}

void test_inferred_indoor_coil_temperature() {
  InferredIndoorCoilTemperature value{};
  const struct Point { uint8_t raw; int16_t deci_celsius; } points[] = {
      {47U, 55}, {75U, 170}, {129U, 341}, {170U, 458}, {186U, 504}, {252U, 721}};
  for (const auto code : {uint8_t{0x81U}, uint8_t{0x87U}}) {
    for (const auto point : points) {
      const uint8_t selector = code == 0x81U ? 0x20U : 0x10U;
      const OperationSegment raw{code, selector, point.raw, 0xA5U};
      value.deci_celsius = -999;
      assert(decode_inferred_indoor_coil_temperature(raw, &value) == OperationValueError::none);
      same_raw(value.raw, raw);
      assert(value.kind == (code == 0x81U ? OperationSemanticKind::thi_r1 : OperationSemanticKind::thi_r3));
      assert(value.scope == OperationScope::per_indoor);
      assert(value.value_quality == (point.raw <= 170U ? OperationValueQuality::inferred_diagnostic_candidate
                                                        : OperationValueQuality::inferred_extrapolated_candidate));
      assert(value.deci_celsius == point.deci_celsius);
      const auto first = value.deci_celsius;
      assert(decode_inferred_indoor_coil_temperature(raw, &value) == OperationValueError::none);
      assert(value.deci_celsius == first);  // deterministic 0.1 C rounding
      const auto classified = decode_operation_segment(raw);
      assert(classified.scope == OperationScope::per_indoor);
      assert(classified.value_quality == OperationValueQuality::bounded_derived_available_later);
      assert(!classified.unit_relation_known);  // classifier is not a calibrated-value claim
    }
  }

  // OP1 is retained, not treated as a universal selector; OP3 is retained and ignored.
  assert(decode_inferred_indoor_coil_temperature({0x81U, 0xE2U, 75U, 0x11U}, &value) == OperationValueError::none);
  assert(value.deci_celsius == 170 && value.raw.op1 == 0xE2U && value.raw.op3 == 0x11U);
  assert(decode_inferred_indoor_coil_temperature({0x87U, 0xA9U, 75U, 0xFEU}, &value) == OperationValueError::none);
  assert(value.deci_celsius == 170 && value.raw.op1 == 0xA9U && value.raw.op3 == 0xFEU);

  for (const auto code : {uint8_t{0x81U}, uint8_t{0x87U}}) {
    const OperationSegment no_value{code, 0x20U, 0U, 0xD4U};
    value.kind = OperationSemanticKind::thi_r1;
    value.scope = OperationScope::per_indoor;
    value.value_quality = OperationValueQuality::inferred_diagnostic_candidate;
    value.deci_celsius = 999;
    assert(decode_inferred_indoor_coil_temperature(no_value, &value) == OperationValueError::out_of_established_range);
    same_raw(value.raw, no_value);
    assert(value.kind == OperationSemanticKind::unknown && value.scope == OperationScope::unknown);
    assert(value.value_quality == OperationValueQuality::unknown_semantic && value.deci_celsius == 0);
  }
  const OperationSegment unsupported{0x82U, 0x20U, 75U, 0x5AU};
  value.deci_celsius = 999;
  assert(decode_inferred_indoor_coil_temperature(unsupported, &value) == OperationValueError::unsupported_segment);
  same_raw(value.raw, unsupported);
  assert(value.deci_celsius == 0 && value.scope == OperationScope::unknown);
  assert(decode_inferred_indoor_coil_temperature({0x13U, 0x10U, 75U, 0}, &value) ==
         OperationValueError::unsupported_segment);
  assert(decode_inferred_indoor_coil_temperature({}, nullptr) == OperationValueError::invalid_argument);

  value.raw = {0x81U, 0x20U, 170U, 0xC3U};
  assert(decode_inferred_indoor_coil_temperature(value.raw, &value) == OperationValueError::none);
  assert(value.deci_celsius == 458 && value.value_quality == OperationValueQuality::inferred_diagnostic_candidate);
  assert(decode_inferred_indoor_coil_temperature({0x81U, 0x20U, 171U, 0xC3U}, &value) == OperationValueError::none);
  assert(value.deci_celsius == 461 && value.value_quality == OperationValueQuality::inferred_extrapolated_candidate);
}
}  // namespace

void test_operation_values() {
  test_inferred_outdoor_candidates();
  test_eev_network_value();
  test_outdoor_coil_raw_value();
  test_tdsh_raw_value();
  test_protection_diagnostic_unknown_guards();
  test_response_no_value_semantics();
  test_inferred_indoor_coil_temperature();
  // Full-table fingerprint over LE16 deci-Celsius, independently fixed from
  // pinned utils.py numeric data (928efd8637bcb346a4945c545ab76733eae6245c).
  uint64_t outdoor_hash = UINT64_C(14695981039346656037);
  uint64_t indoor_hash = outdoor_hash;
  for (unsigned index = 0; index < 256U; ++index) {
    const auto byte = static_cast<uint8_t>(index);
    AmbientTemperature outdoor{}, indoor{};
    const OperationSegment out_raw{0x80U, 0x10U, byte, 0xa5U};
    const OperationSegment in_raw{0x80U, 0x20U, byte, 0x5aU};
    assert(decode_ambient_temperature(out_raw, &outdoor) == OperationValueError::none);
    assert(decode_ambient_temperature(in_raw, &indoor) == OperationValueError::none);
    same_raw(outdoor.raw, out_raw); same_raw(indoor.raw, in_raw);
    assert(outdoor.kind == OperationSemanticKind::outdoor_ambient);
    assert(indoor.kind == OperationSemanticKind::indoor_ambient);
    assert(outdoor.scope == OperationScope::shared_outdoor);
    assert(indoor.scope == OperationScope::per_indoor);
    assert(outdoor.deci_celsius >= -500 && outdoor.deci_celsius <= 430);
    assert(indoor.deci_celsius >= -300 && indoor.deci_celsius <= 520);
    if (index <= 4U) assert(outdoor.deci_celsius == -500);
    if (index <= 15U) assert(indoor.deci_celsius == -300);
    outdoor_hash = fingerprint(outdoor_hash, outdoor.deci_celsius);
    indoor_hash = fingerprint(indoor_hash, indoor.deci_celsius);
    // OP3 is not a second ambient index, but must remain available to callers.
    AmbientTemperature changed{};
    assert(decode_ambient_temperature({0x80U, 0x10U, byte, 0xffU}, &changed) == OperationValueError::none);
    assert(changed.deci_celsius == outdoor.deci_celsius && changed.raw.op3 == 0xffU);
  }
  assert(outdoor_hash == UINT64_C(0xdc8cdcf9d063c5d2));
  assert(indoor_hash == UINT64_C(0x45ee2ca80241bcc2));
  const unsigned indices[] = {0, 4, 5, 15, 16, 64, 128, 192, 255};
  const int16_t outdoor_expected[] = {-500, -500, -489, -340, -330, -80, 97, 252, 430};
  const int16_t indoor_expected[] = {-300, -300, -300, -300, -290, -20, 172, 335, 520};
  for (unsigned i = 0; i < sizeof(indices) / sizeof(indices[0]); ++i) {
    AmbientTemperature value{};
    const auto index = static_cast<uint8_t>(indices[i]);
    assert(decode_ambient_temperature({0x80U, 0x10U, index, 0}, &value) == OperationValueError::none);
    assert(value.deci_celsius == outdoor_expected[i]);
    assert(decode_ambient_temperature({0x80U, 0x20U, index, 0}, &value) == OperationValueError::none);
    assert(value.deci_celsius == indoor_expected[i]);
  }
  AmbientTemperature ambient{};
  assert(decode_ambient_temperature({0x80U, 0x10U, 128U, 0}, &ambient) == OperationValueError::none);
  assert(ambient.deci_celsius == 97 && ambient.deci_celsius * 2 != (128 - 94) * 5);
  assert(decode_ambient_temperature({0x80U, 0x20U, 128U, 0}, &ambient) == OperationValueError::none);
  assert(ambient.deci_celsius == 172 && ambient.deci_celsius * 2 != (128 - 61) * 5);

  for (const uint16_t count : {uint16_t{0}, uint16_t{1}, uint16_t{0x3412}, uint16_t{65535}}) {
    CurrentRunEnergy energy{};
    const OperationSegment raw{0x94U, 0x10U, static_cast<uint8_t>(count), static_cast<uint8_t>(count >> 8U)};
    assert(decode_current_run_energy(raw, &energy) == OperationValueError::none);
    assert(energy.quarter_kwh == count && energy.watt_hours() == static_cast<uint32_t>(count) * 250U);
    same_raw(energy.raw, raw);
  }
  // No lifetime accumulation: a later lower/zero count is returned as-is.
  CurrentRunEnergy energy{};
  assert(decode_current_run_energy({0x94U, 0x10U, 0xffU, 0xffU}, &energy) == OperationValueError::none);
  assert(energy.watt_hours() == 16383750U);
  assert(decode_current_run_energy({0x94U, 0x10U, 0, 0}, &energy) == OperationValueError::none);
  assert(energy.quarter_kwh == 0 && energy.watt_hours() == 0);
  // In-place re-decode of retained raw bytes must survive output reset.
  energy.raw = {0x94U, 0x10U, 1U, 0};
  assert(decode_current_run_energy(energy.raw, &energy) == OperationValueError::none);
  assert(energy.quarter_kwh == 1U);
  ambient.raw = {0x80U, 0x20U, 128U, 0};
  assert(decode_ambient_temperature(ambient.raw, &ambient) == OperationValueError::none);
  assert(ambient.deci_celsius == 172);

  for (unsigned selector = 0; selector < 256U; ++selector) {
    const auto byte = static_cast<uint8_t>(selector);
    if (selector != 0x10U && selector != 0x20U) {
      ambient.kind = OperationSemanticKind::indoor_ambient;
      ambient.scope = OperationScope::per_indoor; ambient.deci_celsius = 123;
      const OperationSegment raw{0x80U, byte, 0x34U, 0x56U};
      assert(decode_ambient_temperature(raw, &ambient) == OperationValueError::unsupported_segment);
      assert(ambient.kind == OperationSemanticKind::unknown && ambient.scope == OperationScope::unknown);
      assert(ambient.deci_celsius == 0); same_raw(ambient.raw, raw);
    }
    if (selector != 0x10U) {
      energy.quarter_kwh = 123;
      const OperationSegment raw{0x94U, byte, 0x34U, 0x56U};
      assert(decode_current_run_energy(raw, &energy) == OperationValueError::unsupported_segment);
      assert(energy.quarter_kwh == 0); same_raw(energy.raw, raw);
      const auto classified = decode_operation_segment(raw);
      assert(classified.kind == OperationSemanticKind::unknown && !classified.unit_relation_known);
      same_raw(classified.raw, raw);
    }
  }
  // Every other code, including raw/UNKNOWN diagnostics, has no value in these
  // typed decoders. Existing classifier matrix is separately regression-tested.
  for (unsigned code = 0; code < 256U; ++code) {
    const OperationSegment raw{static_cast<uint8_t>(code), 0x10U, 0x12U, 0x34U};
    if (code != 0x80U) {
      assert(decode_ambient_temperature(raw, &ambient) == OperationValueError::unsupported_segment);
      same_raw(ambient.raw, raw);
    }
    if (code != 0x94U) {
      assert(decode_current_run_energy(raw, &energy) == OperationValueError::unsupported_segment);
      same_raw(energy.raw, raw);
    }
  }
  assert(decode_ambient_temperature({0x80U, 0x10U, 0, 0}, nullptr) == OperationValueError::invalid_argument);
  assert(decode_current_run_energy({0x94U, 0x10U, 0, 0}, nullptr) == OperationValueError::invalid_argument);
}
