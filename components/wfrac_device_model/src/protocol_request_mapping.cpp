#include "wfrac_device_model/protocol_request_mapping.hpp"

namespace wfrac_device_model {
namespace {

constexpr wfrac_protocol::OperationSegment kNoRequestSentinel{0xFFU, 0xFFU, 0xFFU, 0xFFU};

void carry_command_byte8(const wfrac_protocol::AirconBlockView& latest_receive,
                         wfrac_protocol::CommandState* command_state) {
  (*command_state)[8] = static_cast<uint8_t>(((*command_state)[8] & 0xF7U) | (latest_receive.state[8] & 0x08U));
}

void patch_receive_state(const CommandIntent& intent, wfrac_protocol::CommandState* receive_state) {
  std::visit(
      [receive_state](const auto& typed_intent) {
        using IntentType = std::decay_t<decltype(typed_intent)>;
        if constexpr (std::is_same_v<IntentType, SetPowerIntent>) {
          const uint8_t power = typed_intent.value == PowerState::on ? 0x01U : 0x00U;
          (*receive_state)[2] = static_cast<uint8_t>(((*receive_state)[2] & 0xFCU) | power);
        } else if constexpr (std::is_same_v<IntentType, SetModeIntent>) {
          uint8_t mode = 0U;
          switch (typed_intent.value) {
            case HvacMode::cool: mode = 0x08U; break;
            case HvacMode::heat: mode = 0x10U; break;
            case HvacMode::fan: mode = 0x0CU; break;
            case HvacMode::dry: mode = 0x04U; break;
            default: break;
          }
          (*receive_state)[2] = static_cast<uint8_t>(((*receive_state)[2] & 0xC3U) | mode);
        } else if constexpr (std::is_same_v<IntentType, SetTargetTemperatureIntent>) {
          (*receive_state)[4] = static_cast<uint8_t>(typed_intent.value / 50);
        } else {
          uint8_t fan = 0U;
          switch (typed_intent.value) {
            case FanSetting::auto_mode: fan = 0x07U; break;
            case FanSetting::speed2: fan = 0x01U; break;
            case FanSetting::speed3: fan = 0x02U; break;
            case FanSetting::speed4: fan = 0x06U; break;
            default: break;
          }
          (*receive_state)[3] = static_cast<uint8_t>(((*receive_state)[3] & 0xF0U) | fan);
        }
      },
      intent);
}

}  // namespace

ProtocolRequestMappingError build_control_aircon_stat(const CommandIntent& intent,
                                                      const wfrac_protocol::AirconBlockView& latest_receive,
                                                      wfrac_protocol::MutableByteView output,
                                                      std::size_t* encoded_size) {
  if (encoded_size != nullptr) *encoded_size = 0U;
  if (output.data == nullptr || encoded_size == nullptr) return ProtocolRequestMappingError::invalid_argument;
  if (!latest_receive.crc_valid) return ProtocolRequestMappingError::invalid_receive_baseline;

  wfrac_protocol::CommandState command_state{};
  if (map_command_intent_to_protocol(intent, &command_state) != ProtocolCommandMappingError::none) {
    return ProtocolRequestMappingError::command_mapping_error;
  }
  carry_command_byte8(latest_receive, &command_state);

  wfrac_protocol::CommandState receive_state = latest_receive.state;
  patch_receive_state(intent, &receive_state);

  const wfrac_protocol::AirconBlockInput command{&command_state, &kNoRequestSentinel, 1U};
  const wfrac_protocol::AirconBlockInput receive{&receive_state, &kNoRequestSentinel, 1U};
  if (wfrac_protocol::serialize_aircon_stat(command, receive, output, encoded_size) !=
      wfrac_protocol::AirconStatEncodeError::none) {
    *encoded_size = 0U;
    return ProtocolRequestMappingError::request_encode_error;
  }
  return ProtocolRequestMappingError::none;
}

OperationQueryRequestError build_operation_query_aircon_stat(const uint8_t* codes, std::size_t count,
                                                             const wfrac_protocol::AirconBlockView& latest_receive,
                                                             wfrac_protocol::MutableByteView output,
                                                             std::size_t* encoded_size) {
  if (encoded_size != nullptr) *encoded_size = 0U;
  if (output.data == nullptr || encoded_size == nullptr || codes == nullptr) {
    return OperationQueryRequestError::invalid_argument;
  }
  if (count == 0U || count > 3U) return OperationQueryRequestError::batch_size_out_of_range;
  if (!latest_receive.crc_valid) return OperationQueryRequestError::invalid_receive_baseline;

  wfrac_protocol::OperationSegment command_segments[3]{};
  for (std::size_t index = 0; index < count; ++index) {
    if (!wfrac_protocol::is_known_safe_operation_query_code(codes[index])) {
      return OperationQueryRequestError::invalid_query_code;
    }
    command_segments[index] = wfrac_protocol::make_operation_query_segment(codes[index]);
  }

  wfrac_protocol::CommandState command_state = wfrac_protocol::make_safe_command_state();
  carry_command_byte8(latest_receive, &command_state);
  const wfrac_protocol::CommandState receive_state = latest_receive.state;
  const wfrac_protocol::AirconBlockInput command{&command_state, command_segments, count};
  const wfrac_protocol::AirconBlockInput receive{&receive_state, &kNoRequestSentinel, 1U};
  if (wfrac_protocol::serialize_aircon_stat(command, receive, output, encoded_size) !=
      wfrac_protocol::AirconStatEncodeError::none) {
    *encoded_size = 0U;
    return OperationQueryRequestError::request_encode_error;
  }
  return OperationQueryRequestError::none;
}

}  // namespace wfrac_device_model
