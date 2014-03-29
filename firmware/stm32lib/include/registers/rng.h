/**
 * \ingroup REG
 * \defgroup REGRNG Random number generator
 * @{
 */
#ifndef STM32LIB_INCLUDE_REGISTERS_RNG_H
#define STM32LIB_INCLUDE_REGISTERS_RNG_H

#include <stdint.h>

#define RNG_BASE 0x50060800

typedef struct {
	unsigned : 2;
	unsigned RNGEN : 1;
	unsigned IE : 1;
	unsigned : 28;
} RNG_CR_t;
_Static_assert(sizeof(RNG_CR_t) == 4U, "RNG_CR_t is wrong size");
#define RNG_CR (*(volatile RNG_CR_t *) (RNG_BASE + 0x00))

typedef struct {
	unsigned DRDY : 1;
	unsigned CECS : 1;
	unsigned SECS : 1;
	unsigned : 2;
	unsigned CEIS : 1;
	unsigned SEIS : 1;
	unsigned : 25;
} RNG_SR_t;
_Static_assert(sizeof(RNG_SR_t) == 4U, "RNG_SR_t is wrong size");
#define RNG_SR (*(volatile RNG_SR_t *) (RNG_BASE + 0x04))

#define RNG_DR (*(volatile uint32_t *) (RNG_BASE + 0x08))

#endif

/**
 * @}
 */

