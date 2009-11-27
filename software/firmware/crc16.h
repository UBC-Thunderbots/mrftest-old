#ifndef FIRMWARE_CRC16_H
#define FIRMWARE_CRC16_H

#include <cstddef>
#include <stdint.h>

namespace crc16 {
	uint16_t calculate(const void *, std::size_t) __attribute__((warn_unused_result));
}

#endif

