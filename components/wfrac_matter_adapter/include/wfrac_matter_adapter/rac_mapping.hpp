#pragma once

#include <cstdint>
#include <optional>

#include "wfrac_device_model/device_state.hpp"

namespace wfrac_matter_adapter {

// Project-owned software contract, not a CHIP SDK endpoint or conformance claim.
// Pinned esp-matter: 1e913f65d7f40eb8617be46b195dcacc9a80c165.
constexpr uint32_t room_air_conditioner_device_type = 0x0072U;
constexpr uint8_t room_air_conditioner_device_version = 3U;
enum class RequiredCluster { descriptor, identify, on_off, thermostat };
constexpr RequiredCluster required_clusters[] = {
    RequiredCluster::descriptor, RequiredCluster::identify,
    RequiredCluster::on_off, RequiredCluster::thermostat};
constexpr bool requires_dead_front_behavior = true;

enum class FieldDisposition { value, withhold, null_value, unsupported, invalid_value };

template <typename T>
struct AttributeProjection {
  FieldDisposition disposition = FieldDisposition::withhold;
  // Storage only: never publish this value unless disposition == value.
  T value{};
};

// Only the admitted subset; Auto/FanOnly/Dry are not advertised by this contract.
enum class SystemMode : uint8_t { off = 0x00, cool = 0x03, heat = 0x04 };
enum class SetpointAttribute { occupied_cooling, occupied_heating };

struct RacObservedAttributes {
  AttributeProjection<bool> on_off{};
  AttributeProjection<SystemMode> system_mode{};
  AttributeProjection<int16_t> occupied_cooling_setpoint_centi_celsius{};
  AttributeProjection<int16_t> occupied_heating_setpoint_centi_celsius{};
  AttributeProjection<int16_t> local_temperature_centi_celsius{FieldDisposition::null_value, 0};
  FieldDisposition fan = FieldDisposition::unsupported;
  // Diagnostic metadata, not an invented Matter attribute or online assertion.
  wfrac_device_model::AvailabilityState availability = wfrac_device_model::AvailabilityState::unknown;
};

// Pure observed-state projection; never reads desired/pending or mutates state.
// Withhold means no value for a NON-nullable attribute, not a wire-level NULL.
// LocalTemperature is nullable and uses controller input as an explicit project
// proxy, not a claim of calibrated ambient air temperature. No rounding/clamping.
RacObservedAttributes project_observed_attributes(const wfrac_device_model::DeviceState& state);

enum class MappingError {
  none,
  invalid_argument,
  unsupported,
  observation_unavailable,
  requires_power_on,
  mode_mismatch,
  invalid_temperature,
};

// Pure translation only: no admission/dispatch/acknowledgement or state mutation.
// Non-null output is reset on entry/failure. Caller must submit the resulting
// intent through the existing Device Model/runtime validation and lifecycle.
MappingError map_on_off_write(bool value, std::optional<wfrac_device_model::CommandIntent>* intent);

// Off maps to SetPower(off). Cool/Heat map only to SetMode when an available
// observation already says power on; a single intent cannot also turn power on.
// Unsupported enum values do not become defaults or hidden command sequences.
MappingError map_system_mode_write(uint8_t value, const wfrac_device_model::DeviceState& state,
                                   std::optional<wfrac_device_model::CommandIntent>* intent);

// Only the setpoint matching the observed cool/heat mode maps to the single WF
// target. Exact centi-Celsius is preserved. WF range/0.5 C representability remains
// owned by existing protocol/runtime admission, not duplicated here. No SDK write
// success may be inferred solely from successful translation.
MappingError map_setpoint_write(SetpointAttribute attribute, int32_t centi_celsius,
                                const wfrac_device_model::DeviceState& state,
                                std::optional<wfrac_device_model::CommandIntent>* intent);

}  // namespace wfrac_matter_adapter
