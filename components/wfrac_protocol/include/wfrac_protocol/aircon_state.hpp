#pragma once

#include <array>
#include <cstdint>

#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_protocol {

enum class PowerState { off, on, unknown };
enum class OperationMode { auto_mode, cool, heat, fan, dry, unknown };
enum class FanMode { auto_mode, speed1, speed2, speed3, speed4, unknown };
enum class EncodeError { none, invalid_power, invalid_mode, invalid_fan, invalid_target_temperature };

struct ReceiveState {
  uint8_t model_nr_raw = 0;
  uint8_t power_raw = 0;
  PowerState power = PowerState::unknown;
  uint8_t operation_mode_raw = 0;
  OperationMode operation_mode = OperationMode::unknown;
  uint8_t target_temperature_half_degrees = 0;
  uint8_t fan_raw = 0;
  FanMode fan = FanMode::unknown;
  uint8_t controller_room_temperature_raw = 0;
  int16_t controller_room_temperature_quarter_degrees = 0;
};

using CommandState = std::array<uint8_t, kAirconStateBytes>;

ReceiveState decode_receive_state(const AirconBlockView& receive);
CommandState make_safe_command_state();
EncodeError encode_set_power(PowerState power, CommandState* output);
EncodeError encode_set_mode(OperationMode mode, CommandState* output);
EncodeError encode_set_target_temperature(uint8_t half_degrees, CommandState* output);
EncodeError encode_set_fan(FanMode fan, CommandState* output);

}  // namespace wfrac_protocol
