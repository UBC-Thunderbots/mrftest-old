#include "pid_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class pid_controller_factory : public robot_controller_factory {
		public:
			pid_controller_factory() : robot_controller_factory("PID RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr, bool, unsigned int) {
				robot_controller::ptr p(new pid_controller);
				return p;
			}
	};

	pid_controller_factory factory;

}

pid_controller::pid_controller() : initialized(false), proportional(0.6), differential(0.5), integral(0.0) {
}

void pid_controller::move(const point &current_position, const point &new_position, double current_orientation, double new_orientation, point &linear_velocity, double &angular_velocity) {

	// relative new direction and angle
	const double new_da = angle_mod(new_orientation - current_orientation);
	const point &new_dir = (new_position - current_position).rotate(-current_orientation);

	if (!initialized) {
		initialized = true;
	} else {
	}

	// update the previous
	for (int t = 9; t > 0; --t) {
		error_pos[t] = error_pos[t - 1];
		error_ori[t] = error_ori[t - 1];
	}
	error_pos[0] = new_dir;
	error_ori[0] = new_da;

	point accum_pos(0, 0);
	double accum_ori(0);
	for (int t = 0; t < 10; ++t) {
		accum_pos += error_pos[t];
		accum_ori += error_ori[t];
	}

	linear_velocity = proportional * error_pos[0] + integral * accum_pos + differential * (error_pos[0] - error_pos[1]);
	angular_velocity = proportional * error_ori[0] + integral * accum_ori + differential * (error_ori[0] - error_ori[1]);
}

robot_controller_factory &pid_controller::get_factory() const {
	return factory;
}

