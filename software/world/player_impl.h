#ifndef WORLD_PLAYER_IMPL_H
#define WORLD_PLAYER_IMPL_H

#include <glibmm/refptr.h>
#include "geom/point.h"
#include "util/byref.h"
#include "world/robot_impl.h"

//
// The back-end behind a player object. An implementation of the world must
// provide an implementation of this class and use it to construct player objects
// to pass to the AI. Vectors in this class are in global coordinates.
//
class player_impl : public virtual robot_impl {
	public:
		//
		// A pointer to a player_impl.
		//
		typedef Glib::RefPtr<player_impl> ptr;

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

		//
		// Returns a trivial implementation of player_impl that always leaves
		// the player at the origin facing in the positive X direction.
		//
		static const ptr &trivial();
};

#endif

