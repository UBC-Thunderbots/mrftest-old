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

			robot_controller::ptr create_controller(player_impl::ptr plr, bool, unsigned int) {
				robot_controller::ptr p(new lazy_controller(plr));
				return p;
			}
	};

	lazy_controller_factory factory;

}

lazy_controller::lazy_controller(player_impl::ptr plr) : plr(plr) {
}

void lazy_controller::move(const point &, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const double current_orientation = plr->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = point();
}

robot_controller_factory &lazy_controller::get_factory() const {
	return factory;
}

