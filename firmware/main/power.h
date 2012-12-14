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
	outb(POWER_CTL, inb(POWER_CTL) | 0x02);
}

/**
 * \brief Enables power to the chicker
 */
static inline void power_enable_chicker(void) {
	outb(POWER_CTL, inb(POWER_CTL) | 0x04);
}

/**
 * \brief Opens the chicker emergency relay
 */
static inline void power_open_chicker_emergency_relay(void) {
	outb(POWER_CTL, inb(POWER_CTL) | 0x08);
}

/**
 * \brief Reboots the FPGA
 */
void power_reboot(void) __attribute__((noreturn));

#endif

