#ifndef ROBOT_CONTROLLER_TEST_PLAYER_H
#define ROBOT_CONTROLLER_TEST_PLAYER_H

#include <glibmm.h>
#include "world/player.h"
#include "geom/point.h"
#include "geom/angle.h"



//
// A player object compatible with the robot-controller tester framework.
//
class rc_test_player : public virtual player_impl {
	public:
		typedef Glib::RefPtr<rc_test_player> ptr;
		rc_test_player(point position, double orientation, point linear_velocity, double angular_velocity);
		virtual point position() const;
		virtual double orientation() const;
		virtual bool has_ball() const { return false; }
		virtual point linear_velocity() const;
		virtual double angular_velocity() const;
		virtual void move_impl(const point &linear_velocity, double angular_velocity);
		virtual void dribble(double) {}
		virtual void kick(double) {}
		virtual void chip(double) {}
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

