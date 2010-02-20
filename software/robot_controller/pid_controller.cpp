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

			robot_controller::ptr create_controller(player_impl::ptr plr, bool, unsigned int) const {
				robot_controller::ptr p(new pid_controller(plr));
				return p;
			}
	};

	pid_controller_factory factory;

}

pid_controller::pid_controller(player_impl::ptr plr) : plr(plr), initialized(false),
	proportional_pos(4.0), differential_pos(0.5), integral_pos(0.0),
	proportional_ori(3.0), differential_ori(0.01), integral_ori(0.0) {
}

void pid_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();

	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	const point &new_dir = (new_position - current_position).rotate(-current_orientation);

	if (new_da > PI) new_da -= 2 * PI;

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

	linear_velocity = proportional_pos * error_pos[0] + integral_ori * accum_pos + differential_pos * (error_pos[0] - error_pos[1]);
	angular_velocity = proportional_ori * error_ori[0] + integral_ori * accum_ori + differential_ori * (error_ori[0] - error_ori[1]);
}

robot_controller_factory &pid_controller::get_factory() const {
	return factory;
}

