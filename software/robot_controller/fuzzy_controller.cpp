#include "fuzzy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class fuzzy_controller_factory : public robot_controller_factory {
		public:
			fuzzy_controller_factory() : robot_controller_factory("Fuzzy RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr player, bool, unsigned int) {
				robot_controller::ptr p(new fuzzy_controller (player));
				return p;
			}
	};

	fuzzy_controller_factory factory;

}

//
// The maximum velocity of a robot, in metres per second.
//
const double BOT_MAX_VELOCITY = 5.0;

fuzzy_controller::fuzzy_controller(player_impl::ptr player) {
	robot = player;
}

void fuzzy_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = robot->position();
	const double current_orientation = robot->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	
	double distance_factor = (new_position - current_position).len() / 0.6;
	if (distance_factor > 1) distance_factor = 1;
	
	point velocity_factor = (new_position - current_position).rotate(-current_orientation);
	
	linear_velocity = velocity_factor / velocity_factor.len() * distance_factor * distance_factor * BOT_MAX_VELOCITY;
}

robot_controller_factory &fuzzy_controller::get_factory() const {
	return factory;
}

