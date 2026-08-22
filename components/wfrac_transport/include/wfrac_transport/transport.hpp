#pragma once

#include <cstddef>
#include <cstdint>

namespace wfrac_transport {

struct ByteView {
  const uint8_t* data = nullptr;
  std::size_t size = 0;
};

struct MutableByteView {
  uint8_t* data = nullptr;
  std::size_t size = 0;
};

enum class TransportScheme { http, https };

struct TransportEndpoint {
  TransportScheme scheme = TransportScheme::https;
  const char* host = nullptr;
  uint16_t port = 0;
};

struct TransportRequest {
  TransportEndpoint endpoint{};
  const char* path = nullptr;
  ByteView body{};
  uint32_t timeout_ms = 0;
};

struct TransportResponse {
  int http_status = 0;
  std::size_t body_size = 0;
};

enum class TransportError {
  none,
  invalid_argument,
  timeout,
  connection_failure,
  tls_failure,
  io_failure,
  response_too_large,
};

class ITransport {
 public:
  virtual ~ITransport() = default;

  // Performs one physical attempt. HTTP status remains application-layer data:
  // a received non-2xx response returns TransportError::none.
  virtual TransportError perform(const TransportRequest& request, MutableByteView response_buffer,
                                 TransportResponse* response) = 0;
};

}  // namespace wfrac_transport
