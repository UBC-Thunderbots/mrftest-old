#include "max_power_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

namespace {

	class MaxPowerControllerFactory : public RobotControllerFactory {
		public:
			MaxPowerControllerFactory() : RobotControllerFactory("MAX POWER RC") {
			}

			RefPtr<RobotController2> create_controller(RefPtr<Player> plr, bool, unsigned int) const {
				RefPtr<RobotController2> p(new MaxPowerController(plr));
				return p;
			}
	};

	MaxPowerControllerFactory factory;

}

MaxPowerController::MaxPowerController(RefPtr<Player> plr) : plr(plr) {
}

void MaxPowerController::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
	const Point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
	if (linear_velocity.len()!=0) linear_velocity = linear_velocity/linear_velocity.len() * 9001; // It's over NINE THOUSAAAAAAAND!
}

void MaxPowerController::clear() {
}

RobotControllerFactory &MaxPowerController::get_factory() const {
	return factory;
}

