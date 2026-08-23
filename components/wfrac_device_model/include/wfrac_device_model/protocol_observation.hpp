#pragma once

#include "wfrac_device_model/device_state.hpp"
#include "wfrac_device_model/hvac_state.hpp"
#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_device_model {

enum class ObservationMappingError {
  none,
  invalid_argument,
  invalid_frame,
  crc_mismatch,
};

struct ObservationReconciliationResult {
  ObservationMappingError error = ObservationMappingError::none;
  ReconciliationResult reconciliation{};
};

ObservationMappingError map_aircon_frame_observation(
    const wfrac_protocol::AirconFrameView& frame,
    BasicHvacState* observation);

ObservationMappingError apply_aircon_frame_observation(
    const wfrac_protocol::AirconFrameView& frame,
    DeviceState* state);

ObservationReconciliationResult reconcile_aircon_frame_observation(
    const wfrac_protocol::AirconFrameView& frame,
    DeviceState* state);

}  // namespace wfrac_device_model
