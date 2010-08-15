#include "linear_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

#warning this class needs Doxygen comments in linear_controller.h

namespace {

	class LinearControllerFactory : public RobotControllerFactory {
		public:
			LinearControllerFactory() : RobotControllerFactory("Linear RC") {
			}

			RobotController::Ptr create_controller(Player::Ptr plr, bool, unsigned int) const {
				RobotController::Ptr p(new LinearController(plr));
				return p;
			}
	};

	LinearControllerFactory factory;

}

LinearController::LinearController(Player::Ptr plr) : plr(plr) {
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

