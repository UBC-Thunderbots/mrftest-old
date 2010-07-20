#include "linear_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

namespace {

	class LinearControllerFactory : public RobotControllerFactory {
		public:
			LinearControllerFactory() : RobotControllerFactory("Linear RC") {
			}

			RefPtr<RobotController2> create_controller(RefPtr<Player> plr, bool, unsigned int) const {
				RefPtr<RobotController2> p(new LinearController(plr));
				return p;
			}
	};

	LinearControllerFactory factory;

}

LinearController::LinearController(RefPtr<Player> plr) : plr(plr) {
}

void LinearController::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
	const Point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
}

void LinearController::clear() {
}

RobotControllerFactory &LinearController::get_factory() const {
	return factory;
}

