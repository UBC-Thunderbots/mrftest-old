#ifndef ROBOT_CONTROLLER_TEST_TESTING_RC_H
#define ROBOT_CONTROLLER_TEST_TESTING_RC_H

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

		virtual void move(const point &position, double orientation);		

	private:
		// TODO: add necessary controller fields here
};

#endif

