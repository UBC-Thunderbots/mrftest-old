#ifndef EXTI_H
#define EXTI_H

#include "registers.h"

/**
 * \file
 *
 * \brief Handles the external interrupt pin configuration
 */

/**
 * \brief Sets a mapping for an external interrupt line
 *
 * \param[in] line the EXTI number to configure, from 0 to 15
 *
 * \param[in] port the port to connect the EXTI to, from 0 (PA) to 8 (PI)
 */
static inline void exti_map(unsigned int line, unsigned int port) __attribute__((unused));
static inline void exti_map(unsigned int line, unsigned int port) {
	unsigned int cr = line / 4;
	unsigned int shift = (line % 4) * 4;
	SYSCFG_EXTICR[cr] = (SYSCFG_EXTICR[cr] & ~(0xF << shift)) | (port << shift);
}

#endif

