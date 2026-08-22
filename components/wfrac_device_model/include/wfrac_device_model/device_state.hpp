#pragma once

#include <optional>
#include <type_traits>
#include <variant>

#include "wfrac_device_model/hvac_state.hpp"

namespace wfrac_device_model {

struct ObservedHvacState {
  bool has_value = false;
  BasicHvacState value{};
};

struct DesiredHvacState {
  std::optional<PowerState> power;
  std::optional<HvacMode> mode;
  std::optional<TemperatureCentiC> target_temperature_centi_celsius;
  std::optional<FanSetting> fan;
};

struct SetPowerIntent {
  PowerState value;
};

struct SetModeIntent {
  HvacMode value;
};

struct SetTargetTemperatureIntent {
  TemperatureCentiC value;
};

struct SetFanIntent {
  FanSetting value;
};

using CommandIntent = std::variant<SetPowerIntent, SetModeIntent, SetTargetTemperatureIntent, SetFanIntent>;

struct PendingCommand {
  std::optional<CommandIntent> intent;
};

enum class PendingCommandOutcome { confirmed, rejected, failed, timed_out };
enum class AvailabilityState { unknown, available, unavailable };

enum class DeviceStateError { none, invalid_argument, invalid_control_target, already_pending, no_pending_command };

struct DeviceState {
  ObservedHvacState observed{};
  DesiredHvacState desired{};
  PendingCommand pending{};
  AvailabilityState availability = AvailabilityState::unknown;
};

DeviceStateError apply_desired_intent(const CommandIntent& intent, DesiredHvacState* desired);
DeviceStateError stage_desired_intent(const CommandIntent& intent, DeviceState* state);
DeviceStateError begin_pending_command(const CommandIntent& intent, DeviceState* state);
DeviceStateError finish_pending_command(PendingCommandOutcome outcome, DeviceState* state);

void apply_observation(const BasicHvacState& observation, DeviceState* state);
void clear_all_desired(DeviceState* state);
void set_availability(AvailabilityState availability, DeviceState* state);

}  // namespace wfrac_device_model
