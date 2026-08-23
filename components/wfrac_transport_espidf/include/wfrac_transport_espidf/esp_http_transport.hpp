#pragma once

#include <cstddef>

#include "wfrac_transport/transport.hpp"

namespace wfrac_transport_espidf {

// All pointers are non-owning and must remain valid for each perform() call.
struct EspTlsVerificationConfig {
  const char* server_cert_pem = nullptr;
  std::size_t server_cert_len = 0;
  const char* common_name = nullptr;
};

struct EspHttpTransportConfig {
  EspTlsVerificationConfig tls{};
};

class EspHttpTransport final : public wfrac_transport::ITransport {
 public:
  explicit EspHttpTransport(EspHttpTransportConfig config = {});

  wfrac_transport::TransportError perform(const wfrac_transport::TransportRequest& request,
                                          wfrac_transport::MutableByteView response_buffer,
                                          wfrac_transport::TransportResponse* response) override;

 private:
  EspHttpTransportConfig config_{};
};

}  // namespace wfrac_transport_espidf
