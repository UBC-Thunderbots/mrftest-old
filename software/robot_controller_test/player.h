#ifndef ROBOT_CONTROLLER_TEST_PLAYER_H
#define ROBOT_CONTROLLER_TEST_PLAYER_H

#include "world/player.h"

//
// A player object compatible with the robot-controller tester framework.
//
class rc_test_player : public virtual player {
	public:
		virtual point position() const;
		virtual double orientation() const;
		virtual void move(const point &linear_velocity, double angular_velocity);
		virtual void dribble(double speed __attribute__((__unused__))) {}
		virtual void kick(double strength __attribute__((__unused__))) {}
		virtual void chip(double strength __attribute__((__unused__))) {}

	private:
		// TODO: add necessary player fields here.
};

#endif

