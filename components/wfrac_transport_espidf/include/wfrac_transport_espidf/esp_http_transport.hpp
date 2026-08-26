#pragma once

#include <cstddef>

#include "wfrac_transport/transport.hpp"

namespace wfrac_transport_espidf {

// All pointers are non-owning and must remain valid for each perform() call.
struct EspTlsVerificationConfig {
  // PEM only. A zero length requires a non-empty, NUL-terminated PEM. A
  // nonzero length is project-local validation metadata: it must include the
  // terminal NUL and contain no earlier NUL. It is never forwarded as the
  // ESP-IDF cert_len because v6.0.2 uses nonzero cert_len for its DER path.
  const char* server_cert_pem = nullptr;
  std::size_t server_cert_len = 0;
  const char* common_name = nullptr;

  constexpr bool has_valid_pem_certificate() const {
    if (server_cert_pem == nullptr || server_cert_pem[0] == '\0') return false;
    if (server_cert_len == 0U) return true;
    for (std::size_t index = 0U; index < server_cert_len; ++index) {
      if (server_cert_pem[index] == '\0') return index + 1U == server_cert_len;
    }
    return false;
  }

  constexpr std::size_t esp_idf_cert_len() const { return 0U; }
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
