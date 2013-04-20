#ifndef POWER_H
#define POWER_H

/**
 * \file
 *
 * \brief Controls power to the robot’s subsystems
 */

#include "io.h"

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
 * \brief Enables power to the laser
 */
static inline void power_enable_laser(void) {
	POWER_CTL |= 0x08;
}

/**
 * \brief Reboots the FPGA
 */
void power_reboot(void) __attribute__((noreturn));

#endif

