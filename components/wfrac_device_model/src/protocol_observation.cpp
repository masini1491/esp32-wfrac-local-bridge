#include "wfrac_device_model/protocol_observation.hpp"

#include "wfrac_device_model/protocol_mapping.hpp"
#include "wfrac_protocol/aircon_state.hpp"

namespace wfrac_device_model {
namespace {

bool has_valid_block_layout(const wfrac_protocol::AirconBlockView& block) {
  const std::size_t segment_bytes =
      static_cast<std::size_t>(block.segment_count) * wfrac_protocol::kAirconSegmentBytes;
  const std::size_t expected_size = wfrac_protocol::kAirconStateBytes + 1U + segment_bytes +
                                    wfrac_protocol::kAirconCrcBytes;
  return block.encoded_size == expected_size && block.segments.size == segment_bytes &&
         (segment_bytes == 0U || block.segments.data != nullptr);
}

ObservationMappingError validate_frame(const wfrac_protocol::AirconFrameView& frame) {
  if (!has_valid_block_layout(frame.command) || !has_valid_block_layout(frame.receive) ||
      frame.receive_offset != frame.command.encoded_size) {
    return ObservationMappingError::invalid_frame;
  }
  if (!frame.command.crc_valid || !frame.receive.crc_valid) {
    return ObservationMappingError::crc_mismatch;
  }
  return ObservationMappingError::none;
}

}  // namespace

ObservationMappingError map_aircon_frame_observation(
    const wfrac_protocol::AirconFrameView& frame,
    BasicHvacState* observation) {
  if (observation == nullptr) return ObservationMappingError::invalid_argument;
  *observation = {};
  const ObservationMappingError validation = validate_frame(frame);
  if (validation != ObservationMappingError::none) return validation;
  *observation = map_basic_hvac_state(wfrac_protocol::decode_receive_state(frame.receive));
  return ObservationMappingError::none;
}

ObservationMappingError apply_aircon_frame_observation(
    const wfrac_protocol::AirconFrameView& frame,
    DeviceState* state) {
  if (state == nullptr) return ObservationMappingError::invalid_argument;
  BasicHvacState observation{};
  const ObservationMappingError error = map_aircon_frame_observation(frame, &observation);
  if (error != ObservationMappingError::none) return error;
  apply_observation(observation, state);
  return ObservationMappingError::none;
}

ObservationReconciliationResult reconcile_aircon_frame_observation(
    const wfrac_protocol::AirconFrameView& frame,
    DeviceState* state) {
  ObservationReconciliationResult result{};
  if (state == nullptr) {
    result.error = ObservationMappingError::invalid_argument;
    return result;
  }
  BasicHvacState observation{};
  result.error = map_aircon_frame_observation(frame, &observation);
  if (result.error != ObservationMappingError::none) return result;
  result.reconciliation = reconcile_observation(observation, state);
  return result;
}

}  // namespace wfrac_device_model
