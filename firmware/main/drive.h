#ifndef DRIVE_H
#define DRIVE_H

/**
 * \file
 *
 * \brief Provides the layout of the drive pipe state block.
 */

#include "../shared/drive.h"
#include <stdint.h>

/**
 * \brief The layout of the flags byte of the drive pipe state block.
 */
typedef struct {
	/**
	 * \brief Zero to brake the wheels, or one to run the wheels normally.
	 */
	unsigned enable_wheels : 1;

	/**
	 * \brief Zero to disable the capacitor charger, or one to enable it.
	 */
	unsigned charge : 1;

	/**
	 * \brief Zero to brake the dribbler, or one to spin it.
	 */
	unsigned dribble : 1;

	/**
	 * \brief The mask flag #1 for auto-kick.
	 */
	unsigned autokick_mask1 : 1;

	/**
	 * \brief The mask flag #2 for auto-kick.
	 */
	unsigned autokick_mask2 : 1;

	/**
	 * \brief Whether or not auto-kick is armed.
	 */
	unsigned autokick_armed : 1;
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

	/**
	 * \brief The pulse width for solenoid #1 for auto-kick.
	 */
	uint8_t autokick_pulse1;

	/**
	 * \brief The pulse width for solenoid #2 for auto-kick.
	 */
	uint8_t autokick_pulse2;

	/**
	 * \brief The pulse offset for auto-kick.
	 */
	uint8_t autokick_offset;
} drive_block_t;

/**
 * \brief The current drive block.
 */
extern drive_block_t drive_block;

#endif

