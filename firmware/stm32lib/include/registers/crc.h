#ifndef STM32LIB_INCLUDE_REGISTERS_CRC_H
#define STM32LIB_INCLUDE_REGISTERS_CRC_H

/**
 * \file
 *
 * \brief Defines the CRC calculation unit registers.
 */

#include <stdint.h>

#define CRC_BASE 0x40023000

#define CRC_DR (*(volatile uint32_t *) (CRC_BASE + 0x00))

#define CRC_IDR (*(volatile uint8_t *) (CRC_BASE + 0x04))

typedef struct {
	unsigned RESET : 1;
	unsigned : 31;
} CRC_CR_t;
_Static_assert(sizeof(CRC_CR_t) == 4U, "CRC_CR_t is wrong size");
#define CRC_CR (*(volatile CRC_CR_t *) (CRC_BASE + 0x08))

#endif

