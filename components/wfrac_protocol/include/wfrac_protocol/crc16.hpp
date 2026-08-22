#pragma once

#include <cstddef>
#include <cstdint>

namespace wfrac_protocol {

uint16_t crc16_ccitt(const uint8_t* data, std::size_t length);

}  // namespace wfrac_protocol
