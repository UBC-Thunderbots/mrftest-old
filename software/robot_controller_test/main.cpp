#include "robot_controller/robot_controller.h"
#include "robot_controller_test/player.h"
#include "robot_controller_test/testing_rc.h"
#include "world/player.h"



int main() {
	// Create the player.
	player::ptr player(new rc_test_player);

	// Create the controller under test.
	robot_controller::ptr controller(new testing_rc(player));

	// TODO: exercise the controller

	return 0;
}

