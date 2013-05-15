#ifndef POWER_H
#define POWER_H

/**
 * \file
 *
 * \brief Controls power to the robot’s subsystems
 */

#include "io.h"
#include <stdbool.h>

/**
 * \brief Enables power to the motors
 */
static inline void power_enable_motors(void) {
	POWER_CTL |= 0x02;
}

/**
 * \brief Disables power to the motors
 */
static inline void power_disable_motors(void) {
	POWER_CTL &= ~0x02;
}

/**
 * \brief Reboots the FPGA
 */
void power_reboot(void) __attribute__((noreturn));

/**
 * \brief Checks whether interlocks are overridden
 */
static inline bool interlocks_overridden(void) {
	return !!(POWER_CTL & 0x04);
}

#endif

