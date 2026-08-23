#include <array>
#include <cassert>
#include <cstring>

#include "wfrac_api/api_response.hpp"

namespace { wfrac_protocol::ByteView view(const char* s){return {reinterpret_cast<const uint8_t*>(s),std::strlen(s)};} }
int main(){
  wfrac_api::ApiResponseEnvelopeView env{};
  const char* full=" {\"unknown\":[1,{\"x\":true}],\"result\":429,\"contents\":{\"airconId\":\"ac\\u0031\",\"macAddress\":\"mac\",\"apMode\":2},\"command\":\"getDeviceInfo\"} ";
  assert(wfrac_api::parse_api_response_envelope(view(full),"getDeviceInfo",&env)==wfrac_api::ApiResponseError::none);
  assert(env.result==429&&env.has_contents&&env.contents_json.size>0);
  std::array<uint8_t,4> id{};std::array<uint8_t,4> mac{};wfrac_api::GetDeviceInfoContents info{};
  assert(wfrac_api::parse_get_device_info_contents(env.contents_json,{id.data(),id.size()},{mac.data(),mac.size()},&info)==wfrac_api::ApiResponseError::none);
  assert(std::strcmp(reinterpret_cast<char*>(id.data()),"ac1")==0&&std::strcmp(reinterpret_cast<char*>(mac.data()),"mac")==0&&info.has_ap_mode&&info.ap_mode==2);
  for(const char* bad:{"", "[]", "{\"command\":\"getDeviceInfo\"}", "{\"command\":\"x\",\"result\":0}", "{\"command\":\"getDeviceInfo\",\"command\":\"getDeviceInfo\",\"result\":0}", "{\"command\":\"getDeviceInfo\",\"result\":0}x"}) assert(wfrac_api::parse_api_response_envelope(view(bad),"getDeviceInfo",&env)!=wfrac_api::ApiResponseError::none);
  assert(wfrac_api::parse_api_response_envelope(view("{\"command\":\"getAirconStat\",\"result\":2,\"contents\":null}"),"getAirconStat",&env)==wfrac_api::ApiResponseError::none&&env.result==2&&env.has_contents);
  std::array<uint8_t,3> short_id{};assert(wfrac_api::parse_get_device_info_contents(view("{\"airconId\":\"abc\"}"),{short_id.data(),short_id.size()},{},&info)==wfrac_api::ApiResponseError::output_too_small);
  assert(wfrac_api::parse_get_device_info_contents(view("{\"airconId\":\"\\uD83D\\uDE00\"}"),{id.data(),id.size()},{},&info)==wfrac_api::ApiResponseError::output_too_small);
  assert(wfrac_api::parse_get_device_info_contents(view("{\"airconId\":\"\\uD800\"}"),{id.data(),id.size()},{},&info)==wfrac_api::ApiResponseError::malformed_json);
  return 0;
}
