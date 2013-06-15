#ifndef WHEELS_H
#define WHEELS_H

#include <stdint.h>

/**
 * \brief The modes the wheels can be in.
 */
typedef enum {
	WHEELS_MODE_MANUAL_COMMUTATION,
	WHEELS_MODE_BRAKE,
	WHEELS_MODE_OPEN_LOOP,
	WHEELS_MODE_CLOSED_LOOP,
} wheels_mode_t;

/**
 * \brief The type of a collection of setpoints.
 */
typedef union {
	int16_t wheels[4];
	float robot[3];
} wheels_setpoints_t;

/**
 * \brief The current mode of the wheels.
 */
extern wheels_mode_t wheels_mode;

/**
 * \brief The wheel speed or robot velocity setpoints currently applying.
 */
extern wheels_setpoints_t wheels_setpoints;

/**
 * \brief The most recently read wheel optical encoder counts.
 */
extern int16_t wheels_encoder_counts[4];

/**
 * \brief The most recently calculated drive levels to send to the wheel motors.
 */
extern int16_t wheels_drives[4];

/**
 * \brief Updates the wheels.
 *
 * This function updates \ref wheel_encoder_counts as well as running the control loop (if needed) and sending new power levels to the wheel motors.
 */
void wheels_tick(void);

#endif

