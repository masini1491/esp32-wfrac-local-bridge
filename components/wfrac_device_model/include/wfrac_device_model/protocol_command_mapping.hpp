#pragma once

#include "wfrac_device_model/device_state.hpp"
#include "wfrac_protocol/aircon_state.hpp"

namespace wfrac_device_model {

enum class ProtocolCommandMappingError {
  none,
  invalid_argument,
  invalid_control_target,
  target_temperature_not_representable,
  protocol_encode_error,
};

ProtocolCommandMappingError map_command_intent_to_protocol(const CommandIntent& intent,
                                                           wfrac_protocol::CommandState* output);

}  // namespace wfrac_device_model
