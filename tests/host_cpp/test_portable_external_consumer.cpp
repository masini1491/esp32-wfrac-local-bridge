#include <cstddef>
#include <cstdint>

#include "wfrac_api/api_response.hpp"
#include "wfrac_client/request_executor.hpp"
#include "wfrac_device_model/protocol_mapping.hpp"
#include "wfrac_persistence/records.hpp"
#include "wfrac_protocol/aircon_state.hpp"
#include "wfrac_protocol/crc16.hpp"
#include "wfrac_transport/scheme_policy.hpp"

namespace {

class ConsumerFakeTransport final : public wfrac_transport::ITransport {
 public:
  wfrac_transport::TransportError perform(
      const wfrac_transport::TransportRequest& request,
      wfrac_transport::MutableByteView response_buffer,
      wfrac_transport::TransportResponse* response) override {
    (void)request;
    (void)response_buffer;
    if (response != nullptr) {
      response->http_status = 204;
      response->body_size = 0;
    }
    return wfrac_transport::TransportError::none;
  }
};

}  // namespace

int main() {
  const std::uint8_t protocol_bytes[] = {0x10, 0x20, 0x30};
  const auto crc = wfrac_protocol::crc16_ccitt(protocol_bytes, sizeof(protocol_bytes));

  const wfrac_protocol::ReceiveState receive_state{};
  const auto hvac_state = wfrac_device_model::map_basic_hvac_state(receive_state);
  (void)hvac_state;

  const char response_json[] = "{\"result\":0,\"command\":\"getDeviceInfo\",\"contents\":{}}";
  wfrac_api::ApiResponseEnvelopeView response_envelope{};
  const auto api_error = wfrac_api::parse_api_response_envelope(
      {reinterpret_cast<const std::uint8_t*>(response_json), sizeof(response_json) - 1},
      "getDeviceInfo", &response_envelope);

  wfrac_transport::SchemeCache scheme_cache{};
  const auto scheme_error = wfrac_transport::clear_scheme_cache(&scheme_cache);

  wfrac_api::ApiRequestView request{};
  request.path = "/portable-consumer";
  request.body = {protocol_bytes, sizeof(protocol_bytes)};
  const wfrac_client::ClientEndpoint endpoint{"consumer.invalid", 51443, 1000};
  ConsumerFakeTransport fake_transport;
  std::uint8_t response_storage[8]{};
  wfrac_transport::TransportResponse transport_response{};
  wfrac_client::RequestExecutionReport execution_report{};
  const auto transport_error = wfrac_client::execute_api_request(
      request, endpoint, &scheme_cache, &fake_transport,
      {response_storage, sizeof(response_storage)}, &transport_response,
      &execution_report);

  wfrac_persistence::RecordBuffer record{};
  const auto persistence_error = wfrac_persistence::encode_scheme(
      wfrac_transport::SchemeKnowledge::https_confirmed, &record);

  // Keep every external API call observably referenced in the linked consumer.
  volatile int link_witness = static_cast<int>(crc);
  link_witness += static_cast<int>(api_error);
  link_witness += static_cast<int>(scheme_error);
  link_witness += static_cast<int>(transport_error);
  link_witness += static_cast<int>(persistence_error);
  return link_witness == -1 ? 1 : 0;
}
