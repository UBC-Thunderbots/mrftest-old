#ifndef PLAYER_H
#define PLAYER_H

#include <glibmm.h>
#include "geom/point.h"
#include "util/byref.h"
#include "world/robot.h"

//
// A player that the robot_controller can control.
//
class player : public virtual byref {
	public:
		//
		// A pointer to a player object.
		//
		typedef Glib::RefPtr<player> ptr;

		//
		// Instructs the player to move with specified velocities, in
		// robot-relative coordinates.
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

