#pragma once

#include "wfrac_device_model/protocol_observation.hpp"
#include "wfrac_protocol/aircon_state.hpp"

namespace wfrac_device_model {

// Read-only projection of already-decoded RECEIVE diagnostics. Capability hints,
// writable intents, physical state and Matter attributes are deliberately absent.
struct ExtendedHvacObservation {
  uint8_t model_nr_raw = 0;
  wfrac_protocol::VerticalDirectionState vertical_direction{};
  wfrac_protocol::HorizontalDirectionState horizontal_direction{};
  wfrac_protocol::ErrorObservation error{};
  wfrac_protocol::CoolHotJudgeObservation cool_hot_judge{};
  wfrac_protocol::PerIndoorCompressorDemandObservation per_indoor_compressor_demand{};
  wfrac_protocol::VacantRelatedObservation vacant_related{};
  // Raw-only: protocol evidence does not establish self-clean active/idle state.
  uint8_t self_clean_raw = 0;
};

// Projects only a structurally valid, dual-CRC-valid frame's RECEIVE block.
// This is independent of DeviceState and never changes desired/pending/availability.
ObservationMappingError map_aircon_frame_extended_observation(
    const wfrac_protocol::AirconFrameView& frame,
    ExtendedHvacObservation* observation);

}  // namespace wfrac_device_model
