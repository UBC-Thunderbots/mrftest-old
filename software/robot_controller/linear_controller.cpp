#include "linear_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

namespace {

	class linear_controller_factory : public robot_controller_factory {
		public:
			linear_controller_factory() : robot_controller_factory("Linear RC") {
			}

			robot_controller::ptr create_controller(player::ptr plr, bool, unsigned int) const {
				robot_controller::ptr p(new linear_controller(plr));
				return p;
			}
	};

	linear_controller_factory factory;

}

linear_controller::linear_controller(player::ptr plr) : plr(plr) {
}

void linear_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
}

void linear_controller::clear() {
}

robot_controller_factory &linear_controller::get_factory() const {
	return factory;
}

