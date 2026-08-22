#pragma once

#include "wfrac_device_model/capabilities.hpp"
#include "wfrac_device_model/hvac_state.hpp"
#include "wfrac_protocol/aircon_state.hpp"
#include "wfrac_protocol/capabilities.hpp"

namespace wfrac_device_model {

BasicHvacState map_basic_hvac_state(const wfrac_protocol::ReceiveState& state);
CapabilityHints map_capability_hints(const wfrac_protocol::CapabilityProfile& profile);

}  // namespace wfrac_device_model
