#ifndef ESTOP_H
#define ESTOP_H

/**
 * \file
 *
 * \brief Reads the emergency stop switch.
 */

/**
 * \brief The states the switch can be in.
 */
typedef enum {
	/**
	 * \brief The switch is faulty or not plugged in.
	 */
	ESTOP_BROKEN,

	/**
	 * \brief The switch is set to the stop position.
	 */
	ESTOP_STOP,

	/**
	 * \brief The switch is set to the run position.
	 */
	ESTOP_RUN,
} estop_t;

/**
 * \brief The type of a callback invoked when the state of the emergency stop switch changes.
 */
typedef void (*estop_change_callback_t)(void);

/**
 * \brief Starts reading the emergency stop switch.
 *
 * This function is intended to be called once at application startup.
 * The emergency stop switch will be read continuously from that point forward on a timer.
 */
void estop_init(void);

/**
 * \brief Returns the most recently sampled state of the switch.
 *
 * \return the state of the switch at the most recent sample
 */
estop_t estop_read(void);

/**
 * \brief Sets a callback that will be invoked every time the state of the emergency stop switch changes.
 *
 * \param cb the callback to invoke on switch state change
 */
void estop_set_change_callback(estop_change_callback_t cb);

#endif

