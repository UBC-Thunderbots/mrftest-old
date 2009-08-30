#include "robot_controller_test/testing_rc.h"



testing_rc::testing_rc(player::ptr player) : robot_controller(player) {
}



void testing_rc::move(const point &position, double orientation) {
	// TODO: write this method
	robot->move(point(0.0, 0.0), 0.0);
}

