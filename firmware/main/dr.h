#ifndef DR_H
#define DR_H

#include "log.h"

/**
 * \brief The type of data returned by the dead reckoning module.
 *
 * The following units are used:
 * \li Linear positions are in metres.
 * \li Orientations are in radians.
 * \li Linear velocities are in metres per second.
 * \li Angular velocities are in radians per second.
 *
 * The following coordinate system is used:
 * \li Positive X is in the direction the robot was facing at the last call to
 * \ref dr_reset.
 * \li Positive Y is 90° to the left of positive Y.
 * \li Positive angles are leftward rotation.
 *
 * The origin is the position and orientation of the robot at the last call to
 * \ref dr_reset.
 */
typedef struct {
	/**
	 * \brief The X component of the robot’s accumulated motion.
	 */
	float x;

	/**
	 * \brief The Y component of the robot’s accumulated motion.
	 */
	float y;

	/**
	 * \brief The angular component of the robot’s accumulated motion.
	 */
	float angle;

	/**
	 * \brief The X component of the robot’s velocity.
	 */
	float vx;

	/**
	 * \brief The Y component of the robot’s velocity.
	 */
	float vy;

	/**
	 * \brief The robot’s angular velocity.
	 */
	float avel;
} dr_data_t;

void dr_init(void);
void dr_reset(void);
void dr_tick(log_record_t *log);
void dr_get(dr_data_t *ret);

#endif
