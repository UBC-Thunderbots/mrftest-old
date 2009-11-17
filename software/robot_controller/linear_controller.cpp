#include "linear_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class linear_controller_factory : public robot_controller_factory {
		public:
			linear_controller_factory() : robot_controller_factory("Linear RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr, bool, unsigned int) {
				robot_controller::ptr p(new linear_controller);
				return p;
			}
	};

	linear_controller_factory factory;

}

linear_controller::linear_controller() {
}

void linear_controller::move(const point &current_position, const point &new_position, double current_orientation, double new_orientation, point &linear_velocity, double &angular_velocity) {
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
}

robot_controller_factory &linear_controller::get_factory() const {
	return factory;
}

