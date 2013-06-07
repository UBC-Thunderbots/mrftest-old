#ifndef TSC_H
#define TSC_H

#include "io.h"
#include <stdint.h>

/**
 * \brief Reads the system timestamp counter.
 *
 * \return the number of CPU clock cycles that have passed since system boot, modulo 2<sup>32</sup>
 */
static inline uint32_t rdtsc(void) {
	TSC = 0;
	uint32_t ret = TSC;
	ret <<= 8;
	ret |= TSC;
	ret <<= 8;
	ret |= TSC;
	ret <<= 8;
	ret |= TSC;
	return ret;
}

#endif

