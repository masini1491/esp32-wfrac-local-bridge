#pragma once

#include <cstddef>
#include <cstdint>

#include "wfrac_api/time_provider.hpp"
#include "wfrac_protocol/aircon_frame.hpp"

namespace wfrac_api {

struct ApiClientContext {
  const char* api_version = nullptr;
  const char* device_id = nullptr;
  const char* operator_id = nullptr;
};

struct ApiRequestView {
  const char* path = nullptr;
  wfrac_protocol::ByteView body{};
  int64_t timestamp = 0;
  TimeQuality time_quality = TimeQuality::unavailable;
};

enum class ApiRequestError {
  none,
  invalid_argument,
  time_unavailable,
  invalid_timestamp,
  invalid_aircon_stat,
  output_too_small,
  size_overflow,
  base64_failure,
};

// Caller-supplied strings are required to be valid UTF-8; valid non-ASCII bytes are preserved in JSON output.
ApiRequestError build_get_device_info_request(const ApiClientContext& context, ITimeProvider* time_provider,
                                              wfrac_protocol::MutableByteView output, ApiRequestView* request);
ApiRequestError build_get_aircon_stat_request(const ApiClientContext& context, const char* aircon_id,
                                              ITimeProvider* time_provider, wfrac_protocol::MutableByteView output,
                                              ApiRequestView* request);
ApiRequestError build_set_aircon_stat_request(const ApiClientContext& context, const char* aircon_id,
                                              wfrac_protocol::ByteView raw_aircon_stat, ITimeProvider* time_provider,
                                              wfrac_protocol::MutableByteView output, ApiRequestView* request);
ApiRequestError build_update_account_info_request(const ApiClientContext& context, const char* aircon_id,
                                                  const char* timezone, ITimeProvider* time_provider,
                                                  wfrac_protocol::MutableByteView output, ApiRequestView* request);
ApiRequestError build_delete_account_info_request(const ApiClientContext& context, const char* aircon_id,
                                                  ITimeProvider* time_provider, wfrac_protocol::MutableByteView output,
                                                  ApiRequestView* request);

}  // namespace wfrac_api
