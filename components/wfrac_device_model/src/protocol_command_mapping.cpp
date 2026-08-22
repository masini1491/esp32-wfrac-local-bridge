#include "wfrac_device_model/protocol_command_mapping.hpp"

namespace wfrac_device_model {
namespace {

ProtocolCommandMappingError translate_encode_error(wfrac_protocol::EncodeError error,
                                                   wfrac_protocol::CommandState* output) {
  if (error == wfrac_protocol::EncodeError::none) return ProtocolCommandMappingError::none;
  *output = wfrac_protocol::make_safe_command_state();
  return ProtocolCommandMappingError::protocol_encode_error;
}

ProtocolCommandMappingError map_power(PowerState power, wfrac_protocol::CommandState* output) {
  wfrac_protocol::PowerState protocol_power{};
  if (power == PowerState::off) {
    protocol_power = wfrac_protocol::PowerState::off;
  } else if (power == PowerState::on) {
    protocol_power = wfrac_protocol::PowerState::on;
  } else {
    return ProtocolCommandMappingError::invalid_control_target;
  }
  return translate_encode_error(wfrac_protocol::encode_set_power(protocol_power, output), output);
}

ProtocolCommandMappingError map_mode(HvacMode mode, wfrac_protocol::CommandState* output) {
  wfrac_protocol::OperationMode protocol_mode{};
  switch (mode) {
    case HvacMode::auto_mode: protocol_mode = wfrac_protocol::OperationMode::auto_mode; break;
    case HvacMode::cool: protocol_mode = wfrac_protocol::OperationMode::cool; break;
    case HvacMode::heat: protocol_mode = wfrac_protocol::OperationMode::heat; break;
    case HvacMode::fan: protocol_mode = wfrac_protocol::OperationMode::fan; break;
    case HvacMode::dry: protocol_mode = wfrac_protocol::OperationMode::dry; break;
    default: return ProtocolCommandMappingError::invalid_control_target;
  }
  return translate_encode_error(wfrac_protocol::encode_set_mode(protocol_mode, output), output);
}

ProtocolCommandMappingError map_fan(FanSetting fan, wfrac_protocol::CommandState* output) {
  wfrac_protocol::FanMode protocol_fan{};
  switch (fan) {
    case FanSetting::auto_mode: protocol_fan = wfrac_protocol::FanMode::auto_mode; break;
    case FanSetting::speed1: protocol_fan = wfrac_protocol::FanMode::speed1; break;
    case FanSetting::speed2: protocol_fan = wfrac_protocol::FanMode::speed2; break;
    case FanSetting::speed3: protocol_fan = wfrac_protocol::FanMode::speed3; break;
    case FanSetting::speed4: protocol_fan = wfrac_protocol::FanMode::speed4; break;
    default: return ProtocolCommandMappingError::invalid_control_target;
  }
  return translate_encode_error(wfrac_protocol::encode_set_fan(protocol_fan, output), output);
}

ProtocolCommandMappingError map_target_temperature(TemperatureCentiC centi_celsius,
                                                    wfrac_protocol::CommandState* output) {
  if (centi_celsius < 0 || centi_celsius % 50 != 0) {
    return ProtocolCommandMappingError::target_temperature_not_representable;
  }
  const int16_t half_degrees = static_cast<int16_t>(centi_celsius / 50);
  if (half_degrees > 0x7F) return ProtocolCommandMappingError::target_temperature_not_representable;
  return translate_encode_error(
      wfrac_protocol::encode_set_target_temperature(static_cast<uint8_t>(half_degrees), output), output);
}

}  // namespace

ProtocolCommandMappingError map_command_intent_to_protocol(const CommandIntent& intent,
                                                           wfrac_protocol::CommandState* output) {
  if (output == nullptr) return ProtocolCommandMappingError::invalid_argument;
  *output = wfrac_protocol::make_safe_command_state();

  return std::visit(
      [output](const auto& typed_intent) -> ProtocolCommandMappingError {
        using IntentType = std::decay_t<decltype(typed_intent)>;
        if constexpr (std::is_same_v<IntentType, SetPowerIntent>) {
          return map_power(typed_intent.value, output);
        } else if constexpr (std::is_same_v<IntentType, SetModeIntent>) {
          return map_mode(typed_intent.value, output);
        } else if constexpr (std::is_same_v<IntentType, SetTargetTemperatureIntent>) {
          return map_target_temperature(typed_intent.value, output);
        } else {
          return map_fan(typed_intent.value, output);
        }
      },
      intent);
}

}  // namespace wfrac_device_model
