#ifndef WORLD_PLAYER_H
#define WORLD_PLAYER_H

#include <glibmm/refptr.h>
#include "geom/angle.h"
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
		// Instructs the player to move to the specified position and orientation. Orientation is in radians.
		//
		// Parameters:
		//  new_position
		//   the position to move to
		//
		//  new_orientation
		//   the orientation to move to
		//
		void move(const point &new_position, double new_orientation) {
			impl->move(new_position * (flip ? -1.0 : 1.0), angle_mod(new_orientation + (flip ? PI : 0.0)));
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
		//  impl
		//   the implementation object that provides global coordinates
		//
		//  flip
		//   whether the X and Y coordinates are reversed for this object
		//
		player(player_impl::ptr impl, bool flip) : robot(impl, flip), impl(impl), flip(flip) {
		}

	private:
		player_impl::ptr impl;
		const bool flip;
};

#endif

