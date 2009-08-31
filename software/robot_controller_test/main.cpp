#include <iostream>



#include "robot_controller/robot_controller.h"
#include "robot_controller_test/player.h"
#include "robot_controller_test/testing_rc.h"
#include "world/player.h"

#define EPS 1e-5

int main() {
	// Create an rc_test_player.
	rc_test_player * test_player = new rc_test_player(point(0, 0), 0, point(0, 0), 0);

	// Create the player.
	player::ptr player(test_player);

	// Create the controller under test.
	robot_controller::ptr controller(new testing_rc(player));

	point target_position = point(1, 1);
	double target_orientation = PI / 2;
	for (int i = 0; ; ++i) {
		controller->move(target_position, target_orientation);
		std::cout << i / 30. << " secs at position " << test_player->position() << " with orientation " << test_player->orientation() << std::endl;
		if (abs(test_player->position() - target_position) < EPS && fabs(test_player->orientation() - target_orientation) < EPS) break;
	}

	return 0;
}

