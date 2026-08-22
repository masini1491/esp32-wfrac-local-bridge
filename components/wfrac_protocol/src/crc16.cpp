#include "wfrac_protocol/crc16.hpp"

namespace wfrac_protocol {

uint16_t crc16_ccitt(const uint8_t* data, std::size_t length) {
  uint16_t crc = 0xFFFF;
  for (std::size_t index = 0; index < length; ++index) {
    crc ^= static_cast<uint16_t>(data[index]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U) != 0U ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
                                  : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

}  // namespace wfrac_protocol
