#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_device_model/protocol_command_mapping.hpp"
#include "wfrac_protocol/aircon_stat.hpp"

namespace wfrac_device_model {

enum class ProtocolRequestMappingError {
  none,
  invalid_argument,
  invalid_receive_baseline,
  command_mapping_error,
  request_encode_error,
};

enum class OperationQueryRequestError {
  none,
  invalid_argument,
  invalid_receive_baseline,
  invalid_query_code,
  batch_size_out_of_range,
  request_encode_error,
};

ProtocolRequestMappingError build_control_aircon_stat(const CommandIntent& intent,
                                                      const wfrac_protocol::AirconBlockView& latest_receive,
                                                      wfrac_protocol::MutableByteView output,
                                                      std::size_t* encoded_size);

OperationQueryRequestError build_operation_query_aircon_stat(const uint8_t* codes, std::size_t count,
                                                             const wfrac_protocol::AirconBlockView& latest_receive,
                                                             wfrac_protocol::MutableByteView output,
                                                             std::size_t* encoded_size);

}  // namespace wfrac_device_model
