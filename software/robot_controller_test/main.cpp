#include "geom/angle.h"
#include "robot_controller_test/player.h"
#include "world/player.h"
#include <iostream>

namespace {
	// How close to the endpoint before we consider the task done
	const double EPS = 1e-3;

	// A set of tasks
	const std::pair<point, double> tasks[] =
	{
		std::make_pair(point(10, 0), PI / 2),
		std::make_pair(point(0, 0), PI),
		std::make_pair(point(-10, 0), 0),
		std::make_pair(point(0, 10), PI / 2),
		std::make_pair(point(0, 0), 0),
		std::make_pair(point(0, -10), PI),
		std::make_pair(point(0, 0), 0),
	};

}

const int ntasks = sizeof(tasks) / sizeof(tasks[0]);

double diff_orient(const double& a, const double &b) {
	double angle = fmod(fmod(a - b, 2 * PI) + 2 * PI, 2 * PI);
	if(angle > PI) angle -= 2 * PI;
	return angle;
}

int main() {
	// Create an rc_test_player.
	Glib::RefPtr<rc_test_player> test_player_impl(new rc_test_player(point(0, 0), 0, point(0, 0), 0));
	player::ptr test_player(new player(test_player_impl, false));

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
	robot_controller::ptr controller = factories.find(names[controller_index - 1])->second->create_controller("Blue 1");

	// Attach the controller to the player.
	test_player_impl->set_controller(controller);

	// make the time output nice
	std::cout.precision(4);
	std::cout.setf(std::ios::fixed);

	// used to estimate the robot velocity
	point prev_position(0, 0);
	double prev_orientation(0);
	double total_time(0);

	// Exercise the controller.
	for (int t = 0; t < ntasks; ++t) {
		const point& target_position = tasks[t].first;
		const double& target_orientation = tasks[t].second;
		for (int i = 0; ; ++i) {
			test_player->move(target_position, target_orientation);
			test_player_impl->tick();
			point lin_vel = test_player->position() - prev_position;
			double ang_vel = diff_orient(test_player->orientation(), prev_orientation);
			std::cout << i / 30. << " secs at task " << t << " error pos " << (target_position - test_player->position()) << " ori " << diff_orient(target_orientation, test_player->orientation()) << " linear vel " << lin_vel << " angle vel " << ang_vel << std::endl;
			prev_position = test_player->position();
			prev_orientation = test_player->orientation();
			if ((test_player->position() - target_position).len() < EPS && fabs(diff_orient(test_player->orientation(), target_orientation)) < EPS && lin_vel.len() < EPS && fabs(ang_vel) < EPS) {
				total_time += i / 30.;
				break;
			}
		}
	}

	std::cout << " total time taken " << total_time << std::endl;

	return 0;
}

