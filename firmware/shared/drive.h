#ifndef SHARED_DRIVE_H
#define SHARED_DRIVE_H

/**
 * \file
 *
 * \brief Provides the layout of the drive pipe state block.
 */

#include <stdint.h>

/**
 * \brief The layout of the flags byte of the drive pipe state block.
 */
typedef struct {
	/**
	 * \brief Zero to brake motors and safe capacitors, or one to run normally.
	 */
	unsigned enable_robot : 1;

	/**
	 * \brief Zero to disable the capacitor charger, or one to enable it.
	 */
	unsigned charge : 1;

	/**
	 * \brief Zero to disable the dribbler, or one to enable it.
	 */
	unsigned dribble : 1;
} drive_flags_t;

/**
 * \brief The layout of the drive pipe state block.
 */
typedef struct {
	/**
	 * \brief Miscellaneous operational flags.
	 */
	drive_flags_t flags;

	/**
	 * \brief The requested wheel speeds, in quarters of a degree per five milliseconds.
	 */
	int16_t wheels[4];

	/**
	 * \brief The test mode code.
	 */
	uint8_t test_mode;
} drive_block_t;

#endif

