#ifndef WORLD_PLAYER_H
#define WORLD_PLAYER_H

#include <glibmm/refptr.h>
#include "geom/point.h"
#include "util/byref.h"
#include "world/player_impl.h"
#include "world/robot.h"

//
// A player that the robot_controller can control. Vectors in this class are in
// team coordinates.
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
		void move(const point &linear_velocity, double angular_velocity) {
			impl->move(linear_velocity * (flip ? -1.0 : 1.0), angular_velocity);
		}

		//
		// Instructs the player's dribbler to spin at the specified speed. The
		// speed is between 0 and 1.
		//
		void dribble(double speed) {
			impl->dribble(speed);
		}

		//
		// Instructs the player to kick the ball with the specified strength.
		// The strength is between 0 and 1.
		//
		void kick(double strength) {
			impl->kick(strength);
		}

		//
		// Instructs the player to chip the ball with the specified strength.
		// The strength is between 0 and 1.
		//
		void chip(double strength) {
			impl->chip(strength);
		}

		//
		// Constructs a new player object.
		//
		// Parameters:
		//  id
		//   the global ID number of this robot
		//
		//  impl
		//   the implementation object that provides global coordinates
		//
		//  flip
		//   whether the X and Y coordinates are reversed for this object
		//
		player(unsigned int id, player_impl::ptr impl, bool flip) : robot(id, impl, flip), impl(impl), flip(flip) {
		}

	private:
		player_impl::ptr impl;
		const bool flip;
};

#endif

