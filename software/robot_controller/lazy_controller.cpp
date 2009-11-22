#include "lazy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class lazy_controller_factory : public robot_controller_factory {
		public:
			lazy_controller_factory() : robot_controller_factory("Lazy RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr, bool, unsigned int) {
				robot_controller::ptr p(new lazy_controller);
				return p;
			}
	};

	lazy_controller_factory factory;

}

lazy_controller::lazy_controller() {
}

void lazy_controller::move(const point &current_position, const point &new_position, double current_orientation, double new_orientation, point &linear_velocity, double &angular_velocity) {
	angular_velocity = angle_mod(new_orientation - current_orientation);
}

robot_controller_factory &lazy_controller::get_factory() const {
	return factory;
}

