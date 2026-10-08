#pragma once

#include <array>
#include <cstdint>

#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_protocol {

enum class PowerState { off, on, unknown };
enum class OperationMode { auto_mode, cool, heat, fan, dry, unknown };
enum class FanMode { auto_mode, speed1, speed2, speed3, speed4, unknown };
enum class VerticalSwingState { fixed, swinging, unknown };
enum class VerticalVanePosition { position1, position2, position3, position4, unknown, ignored_while_swinging };
enum class VerticalVaneCommand { swing, position1, position2, position3, position4, unknown };
enum class EncodeError {
  none,
  invalid_power,
  invalid_mode,
  invalid_fan,
  invalid_target_temperature,
  invalid_vertical_vane,
};

struct VerticalDirectionState {
  uint8_t direction_raw = 0;
  uint8_t vane_raw = 0;
  VerticalSwingState swing = VerticalSwingState::unknown;
  VerticalVanePosition position = VerticalVanePosition::unknown;
};

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
  VerticalDirectionState vertical_direction{};
};

using CommandState = std::array<uint8_t, kAirconStateBytes>;

ReceiveState decode_receive_state(const AirconBlockView& receive);
VerticalDirectionState decode_vertical_direction(uint8_t direction_raw, uint8_t vane_raw);
CommandState make_safe_command_state();
EncodeError encode_set_power(PowerState power, CommandState* output);
EncodeError encode_set_mode(OperationMode mode, CommandState* output);
EncodeError encode_set_target_temperature(uint8_t half_degrees, CommandState* output);
EncodeError encode_set_fan(FanMode fan, CommandState* output);
// Encodes one paired byte-2/byte-3 vertical direction selection into a safe
// command baseline; this is not a complete API request or runtime write.
EncodeError encode_set_vertical_vane(VerticalVaneCommand command, CommandState* output);

}  // namespace wfrac_protocol
