#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_api {

enum class ApiResponseError { none, invalid_argument, malformed_json, missing_required_field, unexpected_command, invalid_field_type, integer_overflow, nesting_too_deep, output_too_small };

struct ApiResponseEnvelopeView { int64_t result = 0; bool has_contents = false; wfrac_protocol::ByteView contents_json{}; };

struct GetDeviceInfoContents { std::size_t aircon_id_size = 0; bool has_mac_address = false; std::size_t mac_address_size = 0; bool has_ap_mode = false; int64_t ap_mode = 0; };

ApiResponseError parse_api_response_envelope(wfrac_protocol::ByteView input, const char* expected_command, ApiResponseEnvelopeView* response);
ApiResponseError parse_get_device_info_contents(wfrac_protocol::ByteView contents_json, wfrac_protocol::MutableByteView aircon_id_output, wfrac_protocol::MutableByteView mac_address_output, GetDeviceInfoContents* contents);

}  // namespace wfrac_api
