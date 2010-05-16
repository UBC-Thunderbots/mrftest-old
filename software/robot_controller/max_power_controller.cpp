#include "max_power_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

namespace {

	class max_power_controller_factory : public robot_controller_factory {
		public:
			max_power_controller_factory() : robot_controller_factory("MAX POWER RC") {
			}

			robot_controller::ptr create_controller(player::ptr plr, bool, unsigned int) const {
				robot_controller::ptr p(new max_power_controller(plr));
				return p;
			}
	};

	max_power_controller_factory factory;

}

max_power_controller::max_power_controller(player::ptr plr) : plr(plr) {
}

void max_power_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
	if (linear_velocity.len()!=0) linear_velocity = linear_velocity/linear_velocity.len() * 9001; // It's over NINE THOUSAAAAAAAND!
}

void max_power_controller::clear() {
}

robot_controller_factory &max_power_controller::get_factory() const {
	return factory;
}

