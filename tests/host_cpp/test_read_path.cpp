#include <array>
#include <cassert>
#include <cstdint>
#include <string>

#include "wfrac_api/api_response.hpp"
#include "wfrac_device_model/protocol_observation.hpp"
#include "wfrac_protocol/base64.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {

std::array<uint8_t, 42> make_frame(bool corrupt_receive_crc) {
  std::array<uint8_t, 42> raw{};
  raw[18] = 0U;
  const uint16_t command_crc = wfrac_protocol::crc16_ccitt(raw.data(), 19U);
  raw[19] = static_cast<uint8_t>(command_crc & 0xFFU);
  raw[20] = static_cast<uint8_t>(command_crc >> 8U);
  raw[23] = 0x09U;  // on + cool
  raw[24] = 0x02U;  // speed3
  raw[25] = 47U;    // 23.5 C
  raw[26] = 154U;   // 23.25 C controller room temperature
  raw[39] = 0U;
  const uint16_t receive_crc = wfrac_protocol::crc16_ccitt(raw.data() + 21U, 19U);
  raw[40] = static_cast<uint8_t>(receive_crc & 0xFFU);
  raw[41] = static_cast<uint8_t>(receive_crc >> 8U);
  if (corrupt_receive_crc) raw[41] ^= 1U;
  return raw;
}

std::string encode(const std::array<uint8_t, 42>& raw) {
  std::array<uint8_t, 56> encoded{};
  std::size_t encoded_size = 0U;
  assert(wfrac_protocol::base64_encode({raw.data(), raw.size()}, {encoded.data(), encoded.size()}, &encoded_size) ==
         wfrac_protocol::Base64Error::none);
  return std::string(reinterpret_cast<const char*>(encoded.data()), encoded_size);
}

}  // namespace

int main() {
  const std::string encoded = encode(make_frame(false));
  const std::string response = std::string("{\"command\":\"getAirconStat\",\"result\":2,\"contents\":{") +
                               "\"future\":[{\"ignored\":true}],\"airconStat\":\"" + encoded +
                               "\",\"airconId\":\"synthetic-ac\"}}";
  wfrac_api::ApiResponseEnvelopeView envelope{};
  assert(wfrac_api::parse_api_response_envelope(
             {reinterpret_cast<const uint8_t*>(response.data()), response.size()}, "getAirconStat", &envelope) ==
         wfrac_api::ApiResponseError::none);
  assert(envelope.result == 2);  // Structural parsing does not assign result semantics.

  std::array<uint8_t, 13> aircon_id{};
  std::array<uint8_t, 57> base64_scratch{};
  std::array<uint8_t, 42> raw{};
  wfrac_api::GetAirconStatContents contents{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             envelope.contents_json, {aircon_id.data(), aircon_id.size()},
             {base64_scratch.data(), base64_scratch.size()}, {raw.data(), raw.size()}, &contents) ==
         wfrac_api::ApiResponseError::none);
  assert(contents.frame.command.crc_valid && contents.frame.receive.crc_valid);

  wfrac_device_model::DeviceState state{};
  const auto applied = wfrac_device_model::apply_aircon_frame_observation(contents.frame, &state);
  assert(applied == wfrac_device_model::ObservationMappingError::none);
  assert(state.observed.has_value && state.observed.value.power == wfrac_device_model::PowerState::on &&
         state.observed.value.mode == wfrac_device_model::HvacMode::cool &&
         state.observed.value.fan == wfrac_device_model::FanSetting::speed3 &&
         state.observed.value.target_temperature_centi_celsius == 2350 &&
         state.observed.value.controller_room_temperature_centi_celsius == 2325);

  const std::string bad_encoded = encode(make_frame(true));
  const std::string bad_contents = std::string("{\"airconStat\":\"") + bad_encoded + "\"}";
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(bad_contents.data()), bad_contents.size()}, {},
             {base64_scratch.data(), base64_scratch.size()}, {raw.data(), raw.size()}, &contents) ==
         wfrac_api::ApiResponseError::crc_mismatch);
  return 0;
}
