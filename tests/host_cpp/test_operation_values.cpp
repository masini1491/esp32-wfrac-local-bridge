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
}  // namespace

void test_operation_values() {
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
