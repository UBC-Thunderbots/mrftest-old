#ifndef WORLD_PLAYER_IMPL_H
#define WORLD_PLAYER_IMPL_H

#include "robot_controller/robot_controller.h"
#include "world/robot_impl.h"

//
// The back-end behind a player object. An implementation of the world must
// provide an implementation of this class and use it to construct player objects
// to pass to the AI. Vectors in this class are in global coordinates.
//
class player_impl : public robot_impl {
	public:
		//
		// A pointer to a player_impl.
		//
		typedef Glib::RefPtr<player_impl> ptr;

		//
		// Sets the destination to which the player is trying to move.
		//
		// Parameters:
		//  new_position
		//   the position to move to
		//
		//  new_orientation
		//   the orientation to move to (in radians)
		//
		void move(const point &new_position, double new_orientation) {	
			destination = new_position;
			angular_target = new_orientation;
		}

		//
		// Runs a time tick. This should only be called from the data source
		// (the simulator or the real world driver).
		//
		void tick() {
			if (controller) {
				point linear_velocity;
				double angular_velocity;
				controller->move(position(), destination, orientation(), angular_target, linear_velocity, angular_velocity);
				move_impl(linear_velocity, angular_velocity);
			} else {
				move_impl(point(), 0);
			}
		}

		//
		// Sets the controller used by this robot.
		//
		void set_controller(robot_controller::ptr c) {
			controller = c;
		}

	protected:
		//
		// Instructs the player to move with specified velocities, in
		// robot-relative coordinates that:
		//
		// 	positive x-axis = forward direction of the robot
		// 	positive y-axis = left direction of the robot
		//
		// This function will be called exactly once per time tick, after all AI
		// tick handling has finished (meaning after any calls to dribble(),
		// kick(), and chip()).
		//
		// It is expected that a real-world implementation of move_impl() might
		// enqueue data to send to the robot immediately. It is expected that a
		// simulator implementation of move_impl() will probably save the
		// parameters and only perform actual updates in
		// simulator_engine::tick().
		//
		virtual void move_impl(const point &linear_velocity, double angular_velocity) = 0;

	public:
		//
		// Instructs the player's dribbler to spin at the specified speed. The
		// speed is between 0 and 1.
		//
		// It is expected that an implementation will save the provided value for
		// later consideration.
		//
		virtual void dribble(double speed) = 0;

		//
		// Instructs the player to kick the ball with the specified strength.
		// The strength is between 0 and 1.
		//
		// It is expected that an implementation will save the provided value for
		// later consideration.
		//
		virtual void kick(double strength) = 0;

		//
		// Instructs the player to chip the ball with the specified strength.
		// The strength is between 0 and 1.
		//
		// It is expected that an implementation will save the provided value for
		// later consideration.
		//
		virtual void chip(double strength) = 0;

		//
		// Returns a trivial implementation of player_impl that always leaves
		// the player at the origin facing in the positive X direction.
		//
		static const ptr &trivial();

	private:
		robot_controller::ptr controller;
		point destination;
		double angular_target;
};

#endif

