#include "util/crc16.h"
#include <numeric>

uint16_t CRC16::calculate(uint16_t crc, uint8_t data)
{
    data ^= static_cast<uint8_t>(crc);
    data ^= static_cast<uint8_t>(data << 4);
    crc = static_cast<uint16_t>(crc >> 8);
    crc |= static_cast<uint16_t>(data << 8);
    crc ^= static_cast<uint16_t>(data << 3);
    crc = static_cast<uint16_t>(crc ^ static_cast<uint16_t>(data >> 4));
    return crc;
}

uint16_t CRC16::calculate(const void *buf, std::size_t length, uint16_t crc)
{
    const uint8_t *ptr = static_cast<const uint8_t *>(buf);
    return std::accumulate(
        ptr, ptr + length, crc,
        static_cast<uint16_t (*)(uint16_t, uint8_t)>(&calculate));
}
