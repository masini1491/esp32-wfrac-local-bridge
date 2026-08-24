#include <array>
#include <cassert>
#include <cstring>
#include <string>

#include "wfrac_api/api_response.hpp"
#include "wfrac_protocol/base64.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {

wfrac_protocol::ByteView view(const char* text) {
  return {reinterpret_cast<const uint8_t*>(text), std::strlen(text)};
}

wfrac_protocol::ByteView view(const std::string& text) {
  return {reinterpret_cast<const uint8_t*>(text.data()), text.size()};
}

std::array<uint8_t, 42> valid_frame() {
  std::array<uint8_t, 42> raw{};
  for (std::size_t index = 0; index < 18; ++index) raw[index] = 0xFFU;
  raw[18] = 0U;
  const uint16_t command_crc = wfrac_protocol::crc16_ccitt(raw.data(), 19U);
  raw[19] = static_cast<uint8_t>(command_crc & 0xFFU);
  raw[20] = static_cast<uint8_t>(command_crc >> 8U);
  for (std::size_t index = 0; index < 18; ++index) raw[21U + index] = static_cast<uint8_t>(index);
  raw[39] = 0U;
  const uint16_t receive_crc = wfrac_protocol::crc16_ccitt(raw.data() + 21U, 19U);
  raw[40] = static_cast<uint8_t>(receive_crc & 0xFFU);
  raw[41] = static_cast<uint8_t>(receive_crc >> 8U);
  return raw;
}

std::string encode(const std::array<uint8_t, 42>& raw) {
  std::array<uint8_t, 56> encoded{};
  std::size_t size = 0U;
  assert(wfrac_protocol::base64_encode({raw.data(), raw.size()}, {encoded.data(), encoded.size()}, &size) ==
         wfrac_protocol::Base64Error::none);
  return std::string(reinterpret_cast<const char*>(encoded.data()), size);
}

std::string contents(const std::string& encoded, bool with_id = true) {
  return std::string("{\"unknown\":[{\"nested\":true}],\"airconStat\":\"") + encoded +
         "\"" + (with_id ? ",\"airconId\":\"ac1\"}" : "}");
}

void expect_contents(const std::string& json, wfrac_api::ApiResponseError expected) {
  std::array<uint8_t, 8> id{};
  std::array<uint8_t, 57> scratch{};
  std::array<uint8_t, 42> raw{};
  wfrac_api::GetAirconStatContents parsed{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(json.data()), json.size()}, {id.data(), id.size()},
             {scratch.data(), scratch.size()}, {raw.data(), raw.size()}, &parsed) == expected);
  if (expected != wfrac_api::ApiResponseError::none) {
    assert(!parsed.has_aircon_id && parsed.raw_aircon_stat_size == 0U && parsed.frame.receive_offset == 0U);
  }
}

}  // namespace

int main() {
  wfrac_api::ApiResponseEnvelopeView envelope{};
  const char* device_info =
      " {\"unknown\":[1,{\"x\":true}],\"result\":429,\"contents\":{\"airconId\":\"ac\\u0031\","
      "\"macAddress\":\"mac\",\"apMode\":2},\"command\":\"getDeviceInfo\"} ";
  assert(wfrac_api::parse_api_response_envelope(view(device_info), "getDeviceInfo", &envelope) ==
         wfrac_api::ApiResponseError::none);
  assert(envelope.result == 429 && envelope.has_contents);
  std::array<uint8_t, 4> id{};
  std::array<uint8_t, 4> mac{};
  wfrac_api::GetDeviceInfoContents info{};
  assert(wfrac_api::parse_get_device_info_contents(
             envelope.contents_json, {id.data(), id.size()}, {mac.data(), mac.size()}, &info) ==
         wfrac_api::ApiResponseError::none);
  assert(std::strcmp(reinterpret_cast<char*>(id.data()), "ac1") == 0 &&
         std::strcmp(reinterpret_cast<char*>(mac.data()), "mac") == 0 && info.has_ap_mode && info.ap_mode == 2);
  for (const char* bad : {"", "[]", "{\"command\":\"getDeviceInfo\"}",
                          "{\"command\":\"x\",\"result\":0}",
                          "{\"command\":\"getDeviceInfo\",\"command\":\"getDeviceInfo\",\"result\":0}",
                          "{\"command\":\"getDeviceInfo\",\"result\":0}x"}) {
    assert(wfrac_api::parse_api_response_envelope(view(bad), "getDeviceInfo", &envelope) !=
           wfrac_api::ApiResponseError::none);
  }

  const std::array<uint8_t, 42> frame = valid_frame();
  const std::string encoded = encode(frame);
  assert(encoded.find('/') != std::string::npos);
  const std::string json = contents(encoded);
  std::array<uint8_t, 4> aircon_id{};
  std::array<uint8_t, 57> scratch{};
  std::array<uint8_t, 42> raw{};
  wfrac_api::GetAirconStatContents parsed{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(json.data()), json.size()}, {aircon_id.data(), aircon_id.size()},
             {scratch.data(), scratch.size()}, {raw.data(), raw.size()}, &parsed) ==
         wfrac_api::ApiResponseError::none);
  assert(parsed.has_aircon_id && parsed.aircon_id_size == 3U &&
         std::strcmp(reinterpret_cast<char*>(aircon_id.data()), "ac1") == 0);
  assert(parsed.encoded_aircon_stat_size == encoded.size() && parsed.raw_aircon_stat_size == raw.size());
  assert(parsed.frame.command.crc_valid && parsed.frame.receive.crc_valid &&
         parsed.frame.command.segments.data == raw.data() + 19U);

  const std::string escaped_slash = [&] {
    std::string value = encoded;
    std::size_t index = 0U;
    while ((index = value.find('/', index)) != std::string::npos) {
      value.replace(index, 1U, "\\/");
      index += 2U;
    }
    return value;
  }();
  expect_contents(contents(escaped_slash), wfrac_api::ApiResponseError::none);
  expect_contents("{\"airconStat\":\"abc\"}", wfrac_api::ApiResponseError::invalid_base64);
  expect_contents("{\"airconStat\":\"AA=A\"}", wfrac_api::ApiResponseError::invalid_base64);
  expect_contents("{\"airconStat\":\"\"}", wfrac_api::ApiResponseError::missing_required_field);
  expect_contents("{\"airconId\":\"ac1\"}", wfrac_api::ApiResponseError::missing_required_field);
  expect_contents("{\"airconStat\":null}", wfrac_api::ApiResponseError::invalid_field_type);
  expect_contents("{\"airconStat\":\"AAAA\",\"airconStat\":\"AAAA\"}",
                  wfrac_api::ApiResponseError::malformed_json);
  expect_contents("{\"airconStat\":\"AAAA\",\"airconId\":\"a\",\"airconId\":\"b\"}",
                  wfrac_api::ApiResponseError::malformed_json);

  std::array<uint8_t, 4> exact_id{};
  std::array<uint8_t, 57> exact_scratch{};
  std::array<uint8_t, 42> exact_raw{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(json.data()), json.size()}, {exact_id.data(), exact_id.size()},
             {exact_scratch.data(), exact_scratch.size()}, {exact_raw.data(), exact_raw.size()}, &parsed) ==
         wfrac_api::ApiResponseError::none);
  std::array<uint8_t, 3> short_id{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(json.data()), json.size()}, {short_id.data(), short_id.size()},
             {exact_scratch.data(), exact_scratch.size()}, {exact_raw.data(), exact_raw.size()}, &parsed) ==
         wfrac_api::ApiResponseError::output_too_small);
  std::array<uint8_t, 56> short_scratch{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(json.data()), json.size()}, {}, {short_scratch.data(), short_scratch.size()},
             {exact_raw.data(), exact_raw.size()}, &parsed) == wfrac_api::ApiResponseError::output_too_small);
  const std::string no_id_json = contents(encoded, false);
  std::array<uint8_t, 41> short_raw{};
  assert(wfrac_api::parse_get_aircon_stat_contents(
             {reinterpret_cast<const uint8_t*>(no_id_json.data()), no_id_json.size()}, {},
             {exact_scratch.data(), exact_scratch.size()}, {short_raw.data(), short_raw.size()}, &parsed) ==
         wfrac_api::ApiResponseError::output_too_small);

  std::array<uint8_t, 42> bad_command_crc = frame;
  bad_command_crc[0] ^= 1U;
  expect_contents(contents(encode(bad_command_crc)), wfrac_api::ApiResponseError::crc_mismatch);
  std::array<uint8_t, 42> bad_receive_crc = frame;
  bad_receive_crc[21] ^= 1U;
  expect_contents(contents(encode(bad_receive_crc)), wfrac_api::ApiResponseError::crc_mismatch);
  expect_contents("{\"airconStat\":\"AAAA\"}", wfrac_api::ApiResponseError::invalid_aircon_stat);

  const std::string response = std::string("{\"command\":\"getAirconStat\",\"result\":2,\"contents\":") +
                               no_id_json + "}";
  assert(wfrac_api::parse_api_response_envelope(
             {reinterpret_cast<const uint8_t*>(response.data()), response.size()}, "getAirconStat", &envelope) ==
         wfrac_api::ApiResponseError::none);
  assert(envelope.result == 2);
  assert(wfrac_api::parse_get_aircon_stat_contents(envelope.contents_json, {}, {scratch.data(), scratch.size()},
                                                    {raw.data(), raw.size()}, &parsed) ==
         wfrac_api::ApiResponseError::none);

  assert(wfrac_api::parse_api_response_envelope(
             view("{\"command\":\"getDeviceInfo\",\"result\":   0}"), "getDeviceInfo", &envelope) ==
         wfrac_api::ApiResponseError::none);
  assert(envelope.result == 0);
  wfrac_api::GetDeviceInfoContents spaced_info{};
  assert(wfrac_api::parse_get_device_info_contents(
             view("{\"airconId\":\"ac1\",\"apMode\":   2}"), {id.data(), id.size()}, {}, &spaced_info) ==
         wfrac_api::ApiResponseError::none);
  assert(spaced_info.has_ap_mode && spaced_info.ap_mode == 2);

  for (const char* number : {"0", "-0", "0.5", "-1.25", "1e3", "-2E-2"}) {
    const std::string unknown_number = std::string("{\"unknown\":") + number +
                                       ",\"command\":\"getDeviceInfo\",\"result\":0}";
    assert(wfrac_api::parse_api_response_envelope(view(unknown_number.c_str()), "getDeviceInfo", &envelope) ==
           wfrac_api::ApiResponseError::none);
  }
  for (const char* malformed_number : {"01", "1.", "1e", "+1", ".5"}) {
    const std::string unknown_number = std::string("{\"unknown\":") + malformed_number +
                                       ",\"command\":\"getDeviceInfo\",\"result\":0}";
    assert(wfrac_api::parse_api_response_envelope(view(unknown_number.c_str()), "getDeviceInfo", &envelope) !=
           wfrac_api::ApiResponseError::none);
  }

  for (const char* malformed_string : {"{\"command\":\"getDeviceInfo\\u12\",\"result\":0}",
                                       "{\"command\":\"getDeviceInfo\\uD800\",\"result\":0}",
                                       "{\"command\":\"getDeviceInfo\\"}) {
    assert(wfrac_api::parse_api_response_envelope(view(malformed_string), "getDeviceInfo", &envelope) !=
           wfrac_api::ApiResponseError::none);
  }
  std::array<uint8_t, 4> zero_capacity_storage{};
  zero_capacity_storage.fill(0xA5U);
  const char* identity = "{\"airconId\":\"ac1\"}";
  wfrac_api::GetDeviceInfoContents identity_output{1U, false, 0U, false, 0};
  assert(wfrac_api::parse_get_device_info_contents(
             view(identity), {zero_capacity_storage.data(), 0U}, {}, &identity_output) ==
         wfrac_api::ApiResponseError::output_too_small);
  for (const uint8_t byte : zero_capacity_storage) assert(byte == 0xA5U);
  assert(identity_output.aircon_id_size == 0U && !identity_output.has_mac_address && !identity_output.has_ap_mode);

  const char* embedded_nul = "{\"airconId\":\"a\\u0000b\"}";
  assert(wfrac_api::parse_get_device_info_contents(view(embedded_nul), {id.data(), id.size()}, {}, &info) !=
         wfrac_api::ApiResponseError::none);
  assert(info.aircon_id_size == 0U && !info.has_mac_address && !info.has_ap_mode);

  envelope.result = 123;
  envelope.has_contents = true;
  envelope.contents_json = view("stale");
  assert(wfrac_api::parse_api_response_envelope(view("{\"command\":\"getDeviceInfo\",\"result\":1."),
                                                "getDeviceInfo", &envelope) !=
         wfrac_api::ApiResponseError::none);
  assert(envelope.result == 0 && !envelope.has_contents && envelope.contents_json.data == nullptr);

  const std::string raw_e_acute = "\xC3\xA9";
  const std::string raw_chinese = "\xE4\xB8\xAD\xE6\x96\x87";
  std::array<uint8_t, 8> utf8_id{};
  assert(wfrac_api::parse_get_device_info_contents(
             view(std::string("{\"airconId\":\"") + raw_e_acute + "\"}"),
             {utf8_id.data(), utf8_id.size()}, {}, &info) == wfrac_api::ApiResponseError::none);
  assert(info.aircon_id_size == raw_e_acute.size() &&
         std::memcmp(utf8_id.data(), raw_e_acute.data(), raw_e_acute.size()) == 0);
  assert(wfrac_api::parse_get_device_info_contents(
             view(std::string("{\"airconId\":\"") + raw_chinese + "\"}"),
             {utf8_id.data(), utf8_id.size()}, {}, &info) == wfrac_api::ApiResponseError::none);
  assert(info.aircon_id_size == raw_chinese.size() &&
         std::memcmp(utf8_id.data(), raw_chinese.data(), raw_chinese.size()) == 0);
  assert(wfrac_api::parse_get_device_info_contents(
             view("{\"airconId\":\"\\u00E9\"}"), {utf8_id.data(), utf8_id.size()}, {}, &info) ==
         wfrac_api::ApiResponseError::none);
  assert(info.aircon_id_size == raw_e_acute.size() &&
         std::memcmp(utf8_id.data(), raw_e_acute.data(), raw_e_acute.size()) == 0);
  assert(wfrac_api::parse_api_response_envelope(
             view("{\"command\":\"getDevice\\u0049nfo\",\"result\":0}"), "getDeviceInfo", &envelope) ==
         wfrac_api::ApiResponseError::none);
  assert(wfrac_api::parse_get_device_info_contents(
             view("{\"aircon\\u0049d\":\"ac1\"}"), {id.data(), id.size()}, {}, &info) ==
         wfrac_api::ApiResponseError::none);
  assert(wfrac_api::parse_get_device_info_contents(
             view("{\"airconId\":\"a\",\"aircon\\u0049d\":\"b\"}"),
             {id.data(), id.size()}, {}, &info) == wfrac_api::ApiResponseError::malformed_json);

  for (const std::string& malformed_utf8 : {
           std::string("\xC3\x28", 2), std::string("\xE2\x82", 2), std::string("\xC0\xAF", 2),
           std::string("\xED\xA0\x80", 3), std::string("\xF4\x90\x80\x80", 4)}) {
    const std::string unknown = std::string("{\"unknown\":\"") + malformed_utf8 +
                                "\",\"command\":\"getDeviceInfo\",\"result\":0}";
    assert(wfrac_api::parse_api_response_envelope(view(unknown), "getDeviceInfo", &envelope) ==
           wfrac_api::ApiResponseError::malformed_json);
  }

  {
    std::array<uint8_t, 8> failure_id{};
    std::array<uint8_t, 8> failure_mac{};
    failure_id.fill(0xCCU);
    failure_mac.fill(0xCCU);
    wfrac_api::GetDeviceInfoContents failure_output{3U, true, 3U, true, 7};
    const char* malformed_after_identity = "{\"airconId\":\"ac1\",\"apMode\":1.5}";
    assert(wfrac_api::parse_get_device_info_contents(
               view(malformed_after_identity), {failure_id.data(), failure_id.size()},
               {failure_mac.data(), failure_mac.size()}, &failure_output) ==
           wfrac_api::ApiResponseError::invalid_field_type);
    assert(failure_id[0] == 0U && failure_mac[0] == 0U && failure_output.aircon_id_size == 0U &&
           !failure_output.has_mac_address && !failure_output.has_ap_mode);
  }

  {
    std::array<uint8_t, 8> failure_id{};
    failure_id.fill(0xCCU);
    std::array<uint8_t, 8> failure_scratch{};
    wfrac_api::GetAirconStatContents failure_output{3U, 3U, 42U, 42U, {}};
    const char* malformed_after_identity = "{\"airconId\":\"ac1\",\"airconStat\":null}";
    assert(wfrac_api::parse_get_aircon_stat_contents(
               view(malformed_after_identity), {failure_id.data(), failure_id.size()},
               {failure_scratch.data(), failure_scratch.size()}, {}, &failure_output) ==
           wfrac_api::ApiResponseError::invalid_field_type);
    assert(failure_id[0] == 0U && !failure_output.has_aircon_id && failure_output.aircon_id_size == 0U &&
           failure_output.raw_aircon_stat_size == 0U && failure_output.frame.receive_offset == 0U);
  }
  return 0;
}
