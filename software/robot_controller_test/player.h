#ifndef ROBOT_CONTROLLER_TEST_PLAYER_H
#define ROBOT_CONTROLLER_TEST_PLAYER_H

#include "world/player.h"
#include "geom/point.h"
#include "geom/angle.h"



//
// A player object compatible with the robot-controller tester framework.
//
class rc_test_player : public virtual player {
	public:
		rc_test_player(point position, double orientation, point linear_velocity, double angular_velocity);
		virtual point position() const;
		virtual double orientation() const;
		virtual point linear_velocity() const;
		virtual double angular_velocity() const;
		virtual void move(const point &linear_velocity, double angular_velocity);
		virtual void dribble(double speed __attribute__((__unused__))) {}
		virtual void kick(double strength __attribute__((__unused__))) {}
		virtual void chip(double strength __attribute__((__unused__))) {}
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
