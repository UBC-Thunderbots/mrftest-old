#include "lazy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

#warning this class needs Doxygen comments in its header file

namespace {

	class LazyControllerFactory : public RobotControllerFactory {
		public:
			LazyControllerFactory() : RobotControllerFactory("Lazy RC") {
			}

			RobotController::Ptr create_controller(Player::Ptr plr, bool, unsigned int) const {
				RobotController::Ptr p(new LazyController(plr));
				return p;
			}
	};

	LazyControllerFactory factory;

}

LazyController::LazyController(Player::Ptr plr) : plr(plr) {
}

void LazyController::move(const Point &, double new_orientation, Point &linear_velocity, double &angular_velocity) {
	const double current_orientation = plr->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	linear_velocity = Point();
}

void LazyController::clear() {
}

RobotControllerFactory &LazyController::get_factory() const {
	return factory;
}

