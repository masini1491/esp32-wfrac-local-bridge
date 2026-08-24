#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_api {

enum class ApiResponseError {
  none,
  invalid_argument,
  malformed_json,
  missing_required_field,
  unexpected_command,
  invalid_field_type,
  integer_overflow,
  nesting_too_deep,
  output_too_small,
  invalid_base64,
  invalid_aircon_stat,
  crc_mismatch,
};

struct ApiResponseEnvelopeView { int64_t result = 0; bool has_contents = false; wfrac_protocol::ByteView contents_json{}; };

struct GetDeviceInfoContents { std::size_t aircon_id_size = 0; bool has_mac_address = false; std::size_t mac_address_size = 0; bool has_ap_mode = false; int64_t ap_mode = 0; };

struct GetAirconStatContents {
  bool has_aircon_id = false;
  std::size_t aircon_id_size = 0;
  std::size_t encoded_aircon_stat_size = 0;
  std::size_t raw_aircon_stat_size = 0;
  wfrac_protocol::AirconFrameView frame{};
};

ApiResponseError parse_api_response_envelope(wfrac_protocol::ByteView input, const char* expected_command, ApiResponseEnvelopeView* response);
// On failure, identity output buffers are reset to an empty C string when writable; contents is reset atomically.
ApiResponseError parse_get_device_info_contents(wfrac_protocol::ByteView contents_json, wfrac_protocol::MutableByteView aircon_id_output, wfrac_protocol::MutableByteView mac_address_output, GetDeviceInfoContents* contents);

// On success, frame segment views refer to raw_aircon_stat_output. Keep that
// caller-owned buffer valid while using the returned GetAirconStatContents. On
// failure, contents and the optional airconId output are reset; scratch/raw
// work-buffer contents are unspecified and must not be used.
ApiResponseError parse_get_aircon_stat_contents(
    wfrac_protocol::ByteView contents_json,
    wfrac_protocol::MutableByteView aircon_id_output,
    wfrac_protocol::MutableByteView base64_scratch,
    wfrac_protocol::MutableByteView raw_aircon_stat_output,
    GetAirconStatContents* contents);

}  // namespace wfrac_api
