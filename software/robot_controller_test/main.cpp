#include "robot_controller/robot_controller.h"
#include "robot_controller_test/player.h"
#include "robot_controller_test/testing_rc.h"
#include "world/player.h"
#include <iostream>
#include <cmath>



#define EPS 1e-5

int main() {
	// Create an rc_test_player.
	rc_test_player test_player_impl(point(0, 0), 0, point(0, 0), 0);
	player::ptr test_player(new player(test_player_impl, 1));

	// Create the controller under test.
	robot_controller::ptr controller(new testing_rc(test_player));

	point target_position(1, 1);
	double target_orientation = PI / 2;
	for (int i = 0; ; ++i) {
		controller->move(target_position, target_orientation);
		std::cout << i / 30. << " secs at position " << test_player->position() << " with orientation " << test_player->orientation() << std::endl;
		if (std::abs(test_player->position() - target_position) < EPS && std::fabs(test_player->orientation() - target_orientation) < EPS) break;
	}

	return 0;
}

