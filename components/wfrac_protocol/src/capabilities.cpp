#include "wfrac_protocol/capabilities.hpp"

namespace wfrac_protocol {
namespace {

struct CapabilityEntry {
  uint8_t raw;
  CapabilityGroup group;
  CapabilityFlags flags;
};

constexpr CapabilityEntry kCapabilityTable[] = {
    {0U, CapabilityGroup::separate_2021,
     {true, false, false, true, true, true, true, false, true, true, false, false, false, true, true, false, false}},
    {1U, CapabilityGroup::global_2022,
     {true, true, true, true, true, false, true, true, true, true, false, false, true, false, true, false, false}},
    {2U, CapabilityGroup::high_end_for_japanese_2023,
     {true, false, true, true, true, false, false, false, true, true, false, true, false, false, true, false, false}},
    {3U, CapabilityGroup::zt_2025,
     {false, true, true, true, true, false, true, true, true, true, false, false, true, false, true, true, true}},
    {64U, CapabilityGroup::fdt_2023,
     {false, false, false, false, false, true, false, false, false, true, true, false, true, false, false, false, false}},
};

}  // namespace

CapabilityProfile decode_capabilities(uint8_t model_nr_raw) {
  CapabilityProfile result{};
  result.model_nr_raw = model_nr_raw;
  for (const auto& entry : kCapabilityTable) {
    if (entry.raw == model_nr_raw) {
      result.profile_known = true;
      result.group = entry.group;
      result.flags = entry.flags;
      break;
    }
  }
  return result;
}

}  // namespace wfrac_protocol
