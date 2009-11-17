#include "fuzzy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class fuzzy_controller_factory : public robot_controller_factory {
		public:
			fuzzy_controller_factory() : robot_controller_factory("Lazy RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr, bool, unsigned int) {
				robot_controller::ptr p(new fuzzy_controller);
				return p;
			}
	};

	fuzzy_controller_factory factory;

}

fuzzy_controller::fuzzy_controller() {
}

void fuzzy_controller::move(const point &current_position, const point &new_position, double current_orientation, double new_orientation, point &linear_velocity, double &angular_velocity) {
	// for now, don't move, just face the right direction.
	angular_velocity = angle_mod(new_orientation - current_orientation);
}

robot_controller_factory &fuzzy_controller::get_factory() const {
	return factory;
}

