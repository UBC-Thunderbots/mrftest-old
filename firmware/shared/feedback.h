#ifndef SHARED_FEEDBACK_H
#define SHARED_FEEDBACK_H

/**
 * \file
 *
 * \brief Provides the layout of the feedback state block.
 */

#include "faults.h"
#include <stdint.h>

/**
 * \brief The layout of the flags byte of the feedback state block.
 */
typedef struct {
	/**
	 * \brief Always one to indicate the feedback block is valid.
	 */
	unsigned valid : 1;

	/**
	 * \brief Zero if the ball is not in the break beam, or one if it is.
	 */
	unsigned ball_in_beam : 1;

	/**
	 * \brief Zero if the capacitor is not fully charged, or one if it is.
	 */
	unsigned capacitor_charged : 1;

	/**
	 * \brief Zero if encoder #1 is working properly, or one if it has failed to commutate.
	 */
	unsigned encoder_1_stuck : 1;

	/**
	 * \brief Zero if encoder #2 is working properly, or one if it has failed to commutate.
	 */
	unsigned encoder_2_stuck : 1;

	/**
	 * \brief Zero if encoder #3 is working properly, or one if it has failed to commutate.
	 */
	unsigned encoder_3_stuck : 1;

	/**
	 * \brief Zero if encoder #4 is working properly, or one if it has failed to commutate.
	 */
	unsigned encoder_4_stuck : 1;

	/**
	 * \brief Zero if a motor's hall sensors are in an invalid state (suggesting a line is stuck), or one if they are not.
	 */
	unsigned hall_stuck : 1;
} feedback_flags_t;

/**
 * \brief The layout of the feedback state block.
 */
typedef struct {
	/**
	 * \brief Miscellaneous flags.
	 */
	feedback_flags_t flags;

	/**
	 * \brief The raw ADC reading of the battery voltage monitor.
	 */
	uint16_t battery_voltage_raw;

	/**
	 * \brief The raw ADC reading of the capacitor voltage monitor.
	 */
	uint16_t capacitor_voltage_raw;

	/**
	 * \brief The raw ADC reading of the dribbler thermistor.
	 */
	uint16_t dribbler_temperature_raw;

	/**
	 * \brief The raw ADC reading from the break beam.
	 */
	uint16_t break_beam_raw;
} feedback_block_t;

#endif

