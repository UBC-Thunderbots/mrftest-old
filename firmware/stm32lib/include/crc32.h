#ifndef STM32LIB_CRC32_H
#define STM32LIB_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_be(const void *data, size_t length);

#endif
