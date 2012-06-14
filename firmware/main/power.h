#ifndef POWER_H
#define POWER_H

/**
 * \file
 *
 * \brief Controls power to the robot’s subsystems
 */

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

#endif

