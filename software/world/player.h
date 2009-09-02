#ifndef WORLD_PLAYER_H
#define WORLD_PLAYER_H

#include <glibmm.h>
#include "geom/point.h"
#include "util/byref.h"
#include "world/robot.h"

//
// A player that the robot_controller can control.
//
class player : public virtual byref, public virtual robot {
	public:
		//
		// A pointer to a player object.
		//
		typedef Glib::RefPtr<player> ptr;

		//
		// Instructs the player to move with specified velocities, in
		// robot-relative coordinates that:
		//
		// 	positive x-axis = forward direction of the robot
		// 	positive y-axis = left direction of the robot
		//
		virtual void move(const point &linear_velocity, double angular_velocity) = 0;

		//
		// Instructs the player's dribbler to spin at the specified speed. The
		// speed is between 0 and 1.
		//
		virtual void dribble(double speed) = 0;

		//
		// Instructs the player to kick the ball with the specified strength.
		// The strength is between 0 and 1.
		//
		virtual void kick(double strength) = 0;

		//
		// Instructs the player to chip the ball with the specified strength.
		// The strength is between 0 and 1.
		//
		virtual void chip(double strength) = 0;
};

#endif

