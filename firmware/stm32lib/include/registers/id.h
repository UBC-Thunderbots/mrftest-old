#ifndef STM32LIB_REGISTERS_ID_H
#define STM32LIB_REGISTERS_ID_H

/**
 * \file
 *
 * \brief Defines the device electronic signature registers.
 */

#include <stdint.h>

typedef struct {
	uint32_t L;
	uint32_t M;
	uint32_t H;
} U_ID_t;
_Static_assert(sizeof(U_ID_t) == 12U, "U_ID_t is wrong size");

#define U_ID (*(volatile U_ID_t *) 0x1FFF7A10)
#define F_ID (*(volatile uint16_t *) 0x1FFF7A22)

#endif

