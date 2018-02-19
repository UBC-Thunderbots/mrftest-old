#ifndef PRIMITIVES_MOVE_H
#define PRIMITIVES_MOVE_H

#include "primitive.h"
#ifndef FWSIM
#include "physics.h"
#endif
#include "../physics.h" 
#include "../util/physbot.h"

extern const primitive_t MOVE_PRIMITIVE;

// TODO: Find out actual wheel angle
// this should be the angle between the front of the bot and either
// the closest right or left wheel
static const float CLOSEST_WHEEL_ANGLE = 30.0f * M_PI / 180.0f;

// The minimum distance away from our destination that we must be if we
// are going to rotate the bot onto its wheel axis
static const float APPROACH_LIMIT = 2 * M_PI * ROBOT_RADIUS;

#define VAL_EQUIVALENT_2_ZERO (5e-3f)
#define CONTROL_TICK (1.0f/CONTROL_LOOP_HZ)

#define LOOK_AHEAD_T 10

/**
 * If we are far enough away from our destination, then we should try
 * rotating onto a wheel axis so that we can move faster.
 * 
 * @param pb The data container that housld contain information about
 * the direction the robot will move along.
 * @param angle The angle that the robot is currently facing
 * @return void
 */ 
void choose_rotation_destination(PhysBot *pb, float angle);

/**
 * Calculates the rotation time, velocity, and acceleration to be stored 
 * in a PhysBot data container.
 * 
 * @param pb The data container that has information about major axis time
 * and will store the rotational information
 * @param avel The current rotational velocity of the bot
 * @return void
 */
void plan_move_rotation(PhysBot *pb, float avel);

#endif
