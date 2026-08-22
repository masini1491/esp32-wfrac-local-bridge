#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "wfrac_api/api_request.hpp"
#include "wfrac_protocol/base64.hpp"
#include "wfrac_protocol/crc16.hpp"

namespace {

class FakeTimeProvider final : public wfrac_api::ITimeProvider {
 public:
  wfrac_api::TimeSample now() override {
    ++calls;
    return sample;
  }

  wfrac_api::TimeSample sample{};
  std::size_t calls = 0;
};

constexpr wfrac_api::ApiClientContext kContext{"1.0", "device", "operator"};

std::string body_text(const wfrac_api::ApiRequestView& request) {
  return {reinterpret_cast<const char*>(request.body.data), request.body.size};
}

std::array<uint8_t, 42> valid_aircon_stat() {
  std::array<uint8_t, 42> raw{};
  const uint16_t command_crc = wfrac_protocol::crc16_ccitt(raw.data(), 19U);
  raw[19] = static_cast<uint8_t>(command_crc & 0xFFU);
  raw[20] = static_cast<uint8_t>(command_crc >> 8U);
  const uint16_t receive_crc = wfrac_protocol::crc16_ccitt(raw.data() + 21U, 19U);
  raw[40] = static_cast<uint8_t>(receive_crc & 0xFFU);
  raw[41] = static_cast<uint8_t>(receive_crc >> 8U);
  return raw;
}

void assert_reset(const wfrac_api::ApiRequestView& request) {
  assert(request.path == nullptr && request.body.data == nullptr && request.body.size == 0U && request.timestamp == 0 &&
         request.time_quality == wfrac_api::TimeQuality::unavailable);
}

std::string aircon_stat_base64(const std::string& json) {
  const std::string marker = "\"airconStat\":\"";
  const std::size_t begin = json.find(marker);
  assert(begin != std::string::npos);
  const std::size_t value_begin = begin + marker.size();
  const std::size_t value_end = json.find('"', value_begin);
  assert(value_end != std::string::npos);
  return json.substr(value_begin, value_end - value_begin);
}

}  // namespace

int main() {
  {
    FakeTimeProvider time;
    time.sample = {1700000000, wfrac_api::TimeQuality::trusted};
    std::array<uint8_t, 256> output{};
    wfrac_api::ApiRequestView request{};
    assert(wfrac_api::build_get_device_info_request(kContext, &time, {output.data(), output.size()}, &request) ==
           wfrac_api::ApiRequestError::none);
    assert(std::strcmp(request.path, "/beaver/command/getDeviceInfo") == 0 && time.calls == 1U);
    assert(request.timestamp == 1700000000 && request.time_quality == wfrac_api::TimeQuality::trusted);
    const std::string body = body_text(request);
    assert(body == "{\"apiVer\":\"1.0\",\"command\":\"getDeviceInfo\",\"deviceId\":\"device\",\"operatorId\":\"operator\",\"timestamp\":1700000000}");
    assert(body.find("contents") == std::string::npos);
  }

  {
    FakeTimeProvider time;
    time.sample = {5000000000LL, wfrac_api::TimeQuality::estimated};
    std::array<uint8_t, 256> output{};
    wfrac_api::ApiRequestView request{};
    assert(wfrac_api::build_get_aircon_stat_request(kContext, "aircon", &time, {output.data(), output.size()}, &request) ==
           wfrac_api::ApiRequestError::none);
    assert(std::strcmp(request.path, "/beaver/command/getAirconStat") == 0 && time.calls == 1U);
    assert(request.timestamp == 5000000000LL && request.time_quality == wfrac_api::TimeQuality::estimated);
    const std::string body = body_text(request);
    assert(body.find("\"timestamp\":5000000000") != std::string::npos);
    assert(body.find("\"contents\":{\"airconId\":\"aircon\"}") != std::string::npos);
    assert(body.find("airconStat") == std::string::npos);
  }

  for (const auto sample : {wfrac_api::TimeSample{0, wfrac_api::TimeQuality::trusted},
                            wfrac_api::TimeSample{0, wfrac_api::TimeQuality::estimated},
                            wfrac_api::TimeSample{100, wfrac_api::TimeQuality::unavailable}}) {
    FakeTimeProvider time;
    time.sample = sample;
    std::array<uint8_t, 256> output{};
    wfrac_api::ApiRequestView request{"stale", {output.data(), 1U}, 1, wfrac_api::TimeQuality::trusted};
    const auto result = wfrac_api::build_get_device_info_request(kContext, &time, {output.data(), output.size()}, &request);
    assert(result == (sample.quality == wfrac_api::TimeQuality::unavailable ? wfrac_api::ApiRequestError::time_unavailable
                                                                             : wfrac_api::ApiRequestError::invalid_timestamp));
    assert(time.calls == 1U);
    assert_reset(request);
  }

  {
    FakeTimeProvider time;
    time.sample = {100, wfrac_api::TimeQuality::trusted};
    std::array<uint8_t, 256> output{};
    wfrac_api::ApiRequestView request{};
    const wfrac_api::ApiClientContext invalid{nullptr, "device", "operator"};
    assert(wfrac_api::build_get_device_info_request(invalid, &time, {output.data(), output.size()}, &request) ==
           wfrac_api::ApiRequestError::invalid_argument);
    assert(wfrac_api::build_get_aircon_stat_request(kContext, "", &time, {output.data(), output.size()}, &request) ==
           wfrac_api::ApiRequestError::invalid_argument);
    assert(time.calls == 0U);
  }

  {
    const char api_version[] = {'1', '"', '\\', '\0'};
    const char device_id[] = {'d', '\n', '\t', '\x01', 'x', '\0'};
    const char operator_id[] = {'o', '\r', '\b', '\f', '\0'};
    const wfrac_api::ApiClientContext escaped{api_version, device_id, operator_id};
    FakeTimeProvider time;
    time.sample = {123, wfrac_api::TimeQuality::trusted};
    std::array<uint8_t, 256> output{};
    wfrac_api::ApiRequestView request{};
    assert(wfrac_api::build_get_device_info_request(escaped, &time, {output.data(), output.size()}, &request) ==
           wfrac_api::ApiRequestError::none);
    assert(body_text(request) ==
           "{\"apiVer\":\"1\\\"\\\\\",\"command\":\"getDeviceInfo\",\"deviceId\":\"d\\n\\t\\u0001x\",\"operatorId\":\"o\\r\\b\\f\",\"timestamp\":123}");
  }

  {
    const auto raw = valid_aircon_stat();
    FakeTimeProvider time;
    time.sample = {200, wfrac_api::TimeQuality::trusted};
    std::array<uint8_t, 512> output{};
    wfrac_api::ApiRequestView request{};
    assert(wfrac_api::build_set_aircon_stat_request(kContext, "aircon", {raw.data(), raw.size()}, &time,
                                                    {output.data(), output.size()}, &request) ==
           wfrac_api::ApiRequestError::none);
    assert(std::strcmp(request.path, "/beaver/command/setAirconStat") == 0 && time.calls == 1U);
    const std::string encoded = aircon_stat_base64(body_text(request));
    std::vector<uint8_t> decoded(raw.size());
    std::size_t decoded_size = 0;
    assert(wfrac_protocol::base64_decode({reinterpret_cast<const uint8_t*>(encoded.data()), encoded.size()},
                                         {decoded.data(), decoded.size()}, &decoded_size) ==
           wfrac_protocol::Base64Error::none);
    assert(decoded_size == raw.size() && std::memcmp(decoded.data(), raw.data(), raw.size()) == 0);
  }

  {
    const auto raw = valid_aircon_stat();
    const std::array<std::vector<uint8_t>, 3> malformed{{std::vector<uint8_t>{0U},
                                                           std::vector<uint8_t>(raw.begin(), raw.end()),
                                                           std::vector<uint8_t>(raw.begin(), raw.end())}};
    auto bad_command = malformed[1];
    auto bad_receive = malformed[2];
    bad_command[19] ^= 0x01U;
    bad_receive[40] ^= 0x01U;
    const std::vector<uint8_t>* const candidates[] = {&malformed[0], &bad_command, &bad_receive};
    for (const auto* candidate : candidates) {
      FakeTimeProvider time;
      time.sample = {200, wfrac_api::TimeQuality::trusted};
      std::array<uint8_t, 256> output{};
      wfrac_api::ApiRequestView request{};
      assert(wfrac_api::build_set_aircon_stat_request(kContext, "aircon", {candidate->data(), candidate->size()}, &time,
                                                      {output.data(), output.size()}, &request) ==
             wfrac_api::ApiRequestError::invalid_aircon_stat);
      assert(time.calls == 0U);
      assert_reset(request);
    }
  }

  {
    FakeTimeProvider first_time;
    first_time.sample = {300, wfrac_api::TimeQuality::trusted};
    std::array<uint8_t, 256> large_output{};
    wfrac_api::ApiRequestView first{};
    assert(wfrac_api::build_get_aircon_stat_request(kContext, "aircon", &first_time,
                                                    {large_output.data(), large_output.size()}, &first) ==
           wfrac_api::ApiRequestError::none);
    std::vector<uint8_t> exact(first.body.size);
    FakeTimeProvider exact_time;
    exact_time.sample = {300, wfrac_api::TimeQuality::trusted};
    wfrac_api::ApiRequestView exact_request{};
    assert(wfrac_api::build_get_aircon_stat_request(kContext, "aircon", &exact_time, {exact.data(), exact.size()},
                                                    &exact_request) == wfrac_api::ApiRequestError::none);
    std::vector<uint8_t> short_buffer(first.body.size - 1U);
    FakeTimeProvider short_time;
    short_time.sample = {300, wfrac_api::TimeQuality::trusted};
    wfrac_api::ApiRequestView short_request{"stale", {short_buffer.data(), 1U}, 1, wfrac_api::TimeQuality::trusted};
    assert(wfrac_api::build_get_aircon_stat_request(kContext, "aircon", &short_time,
                                                    {short_buffer.data(), short_buffer.size()}, &short_request) ==
           wfrac_api::ApiRequestError::output_too_small);
    assert(short_time.calls == 1U);
    assert_reset(short_request);
  }

  return 0;
}
