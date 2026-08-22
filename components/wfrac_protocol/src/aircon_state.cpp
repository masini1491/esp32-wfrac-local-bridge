#include "wfrac_protocol/aircon_state.hpp"

namespace wfrac_protocol {
namespace {

PowerState decode_power(uint8_t raw) {
  if (raw == 0x00U) return PowerState::off;
  if (raw == 0x01U) return PowerState::on;
  return PowerState::unknown;
}

OperationMode decode_mode(uint8_t raw) {
  switch (raw) {
    case 0x00U: return OperationMode::auto_mode;
    case 0x04U: return OperationMode::dry;
    case 0x08U: return OperationMode::cool;
    case 0x0CU: return OperationMode::fan;
    case 0x10U: return OperationMode::heat;
    default: return OperationMode::unknown;
  }
}

FanMode decode_fan(uint8_t raw) {
  switch (raw) {
    case 0x07U: return FanMode::auto_mode;
    case 0x00U: return FanMode::speed1;
    case 0x01U: return FanMode::speed2;
    case 0x02U: return FanMode::speed3;
    case 0x06U: return FanMode::speed4;
    default: return FanMode::unknown;
  }
}

}  // namespace

ReceiveState decode_receive_state(const AirconBlockView& receive) {
  ReceiveState result{};
  result.model_nr_raw = static_cast<uint8_t>(receive.state[0] & 0x7FU);
  result.power_raw = static_cast<uint8_t>(receive.state[2] & 0x03U);
  result.power = decode_power(result.power_raw);
  result.operation_mode_raw = static_cast<uint8_t>(receive.state[2] & 0x3CU);
  result.operation_mode = decode_mode(result.operation_mode_raw);
  result.target_temperature_half_degrees = receive.state[4];
  result.fan_raw = static_cast<uint8_t>(receive.state[3] & 0x0FU);
  result.fan = decode_fan(result.fan_raw);
  result.controller_room_temperature_raw = receive.state[5];
  result.controller_room_temperature_quarter_degrees = static_cast<int16_t>(receive.state[5]) - 61;
  return result;
}

CommandState make_safe_command_state() {
  CommandState state{};
  state[5] = 0xFFU;
  return state;
}

EncodeError encode_set_power(PowerState power, CommandState* output) {
  if (output == nullptr) return EncodeError::invalid_power;
  *output = make_safe_command_state();
  if (power == PowerState::off) {
    (*output)[2] = 0x02U;
    return EncodeError::none;
  }
  if (power == PowerState::on) {
    (*output)[2] = 0x03U;
    return EncodeError::none;
  }
  return EncodeError::invalid_power;
}

EncodeError encode_set_mode(OperationMode mode, CommandState* output) {
  if (output == nullptr) return EncodeError::invalid_mode;
  *output = make_safe_command_state();
  switch (mode) {
    case OperationMode::auto_mode: (*output)[2] = 0x20U; return EncodeError::none;
    case OperationMode::dry: (*output)[2] = 0x24U; return EncodeError::none;
    case OperationMode::cool: (*output)[2] = 0x28U; return EncodeError::none;
    case OperationMode::fan: (*output)[2] = 0x2CU; return EncodeError::none;
    case OperationMode::heat: (*output)[2] = 0x30U; return EncodeError::none;
    default: return EncodeError::invalid_mode;
  }
}

EncodeError encode_set_target_temperature(uint8_t half_degrees, CommandState* output) {
  if (output == nullptr || half_degrees > 0x7FU) return EncodeError::invalid_target_temperature;
  *output = make_safe_command_state();
  (*output)[4] = static_cast<uint8_t>(half_degrees | 0x80U);
  return EncodeError::none;
}

EncodeError encode_set_fan(FanMode fan, CommandState* output) {
  if (output == nullptr) return EncodeError::invalid_fan;
  *output = make_safe_command_state();
  switch (fan) {
    case FanMode::auto_mode: (*output)[3] = 0x0FU; return EncodeError::none;
    case FanMode::speed1: (*output)[3] = 0x08U; return EncodeError::none;
    case FanMode::speed2: (*output)[3] = 0x09U; return EncodeError::none;
    case FanMode::speed3: (*output)[3] = 0x0AU; return EncodeError::none;
    case FanMode::speed4: (*output)[3] = 0x0EU; return EncodeError::none;
    default: return EncodeError::invalid_fan;
  }
}

}  // namespace wfrac_protocol
