#ifndef ROBOT_CONTROLLER_TEST_PLAYER_H
#define ROBOT_CONTROLLER_TEST_PLAYER_H

#include "world/player_impl.h"

//
// A player object compatible with the robot-controller tester framework.
//
class rc_test_player : public player_impl {
	public:
		typedef Glib::RefPtr<rc_test_player> ptr;
		rc_test_player(point position, double orientation, point linear_velocity, double angular_velocity);
		point position() const;
		double orientation() const;
		bool has_ball() const { return false; }
		point linear_velocity() const;
		double angular_velocity() const;
		void move_impl(const point &linear_velocity, double angular_velocity);
		void dribble(double) {}
		void kick(double) {}
		void chip(double) {}
		void ui_set_position(const point &) {}
	private:
		point pos;
		double ori;
		point lin_vel;
		double ang_vel;
		int sub_steps;
		double dt;
		double time_step;

		double max_angular_velocity;
		double max_angular_velocity_accel;
		
		double max_linear_velocity;
		double max_linear_velocity_accel;
};

#endif

