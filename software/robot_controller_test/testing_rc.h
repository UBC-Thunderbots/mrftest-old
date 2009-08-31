#ifndef ROBOT_CONTROLLER_TEST_TESTING_RC_H
#define ROBOT_CONTROLLER_TEST_TESTING_RC_H

#include "geom/point.h"
#include "geom/angle.h"
#include "robot_controller/robot_controller.h"
#include "world/player.h"

//
// The robot controller being tested.
//
class testing_rc : public virtual robot_controller {
	public:
		//
		// Constructs a new controller.
		//
		testing_rc(player::ptr player);
			
		//
		// Constructs a new controller.
		//
		// Parameters:
		//  target_position
    		//   location the player wants to be in world coordinate
		//
		//  target_orientation 
		//   direction the player wants to have
		//
		virtual void move(const point &target_position, double target_orientation);		

	private:
		double get_velocity(double d, double v0, double v1, double max_vel, double max_accel);
		// double get_velocity_with_time(double s, double v0, double v1, double max_vel, double max_accel, double time) {

		point old_position;
		double old_orientation;

		double time_step;

		double max_angular_velocity;
		double max_angular_velocity_accel;

		double max_linear_velocity;
		double max_linear_velocity_accel;  
};

#endif

