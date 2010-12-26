#ifndef DONGLE_STATUS_H
#define DONGLE_STATUS_H

#include <stdint.h>

/**
 * \brief The possible states the emergency stop switch can be in.
 */
typedef enum {
	/**
	 * \brief The switch has not been sampled yet.
	 */
	ESTOP_STATE_UNINITIALIZED = 0,

	/**
	 * \brief The switch is not plugged in or is not working properly.
	 */
	ESTOP_STATE_DISCONNECTED = 1,

	/**
	 * \brief The switch is in the stop position.
	 */
	ESTOP_STATE_STOP = 2,

	/**
	 * \brief The switch is in the run position.
	 */
	ESTOP_STATE_RUN = 3,
} estop_state_t;

/**
 * \brief The layout of a dongle status packet.
 */
typedef struct {
	/**
	 * \brief The current state of the emergency stop switch.
	 */
	estop_state_t estop;

	/**
	 * \brief The bit mask of XBee statuses.
	 *
	 * Bits 0 and 1 indicate if the corresponding XBee is ready to use.
	 * Bits 2 and 3 indicate if the corresponding XBee has failed.
	 * Bits 4 and 5 indicate if the corresponding XBee is currently being initialized.
	 */
	uint8_t xbees;

	/**
	 * \brief The mask of robot responsiveness.
	 */
	uint16_t robots;
} dongle_status_t;

/**
 * \brief The dongle status block that application components should write to.
 */
extern volatile dongle_status_t dongle_status;

/**
 * \brief Starts reporting dongle status updates over USB.
 */
void dongle_status_start(void);

/**
 * \brief Stops reporting dongle status updates over USB.
 */
void dongle_status_stop(void);

/**
 * \brief Halts the dongle status endpoint.
 */
void dongle_status_halt(void);

/**
 * \brief Unhalts the dongle status endpoint.
 */
void dongle_status_unhalt(void);

/**
 * \brief This function must be called after the application writes to dongle_status.
 */
void dongle_status_dirty(void);

#endif

