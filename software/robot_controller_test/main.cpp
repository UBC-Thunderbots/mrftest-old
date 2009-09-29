#include "robot_controller/robot_controller.h"
#include "robot_controller_test/player.h"
#include "world/player.h"
#include <iostream>
#include <cmath>



#define EPS 1e-5

int main() {
	// Create an rc_test_player.
	Glib::RefPtr<rc_test_player> test_player_impl(new rc_test_player(point(0, 0), 0, point(0, 0), 0));
	player::ptr test_player(new player(0, test_player_impl, false));

	// Create the controller under test.
	const std::map<Glib::ustring, robot_controller_factory *> &factories = robot_controller_factory::all();
	std::vector<Glib::ustring> names;
	for (std::map<Glib::ustring, robot_controller_factory *>::const_iterator i = factories.begin(), iend = factories.end(); i != iend; ++i)
		names.push_back(i->first);
	std::cout << "These controllers are available:\n";
	for (unsigned int i = 0; i < names.size(); i++)
		std::cout << '[' << (i + 1) << "]\t" << Glib::locale_from_utf8(names[i]) << '\n';
	std::cout << "Enter a number from the list: ";
	std::cout.flush();
	unsigned int controller_index;
	std::cin >> controller_index;
	if (controller_index < 1 || controller_index > names.size()) {
		std::cerr << "Invalid controller number chosen.\n";
		return 1;
	}
	robot_controller::ptr controller = factories.find(names[controller_index - 1])->second->create_controller();

	// Attach the controller to the player.
	test_player->set_controller(controller);

	// Exercise the controller.
	point target_position(1, 1);
	double target_orientation = PI / 2;
	for (int i = 0; ; ++i) {
		test_player->move(target_position, target_orientation);
		std::cout << i / 30. << " secs at position " << test_player->position() << " with orientation " << test_player->orientation() << std::endl;
		if ((test_player->position() - target_position).len() < EPS && std::fabs(test_player->orientation() - target_orientation) < EPS) break;
	}

	return 0;
}

