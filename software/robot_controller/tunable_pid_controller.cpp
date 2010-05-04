#include "tunable_pid_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

#define REDUCED_PARAMS

namespace {

	// full configurations
	// params = 9
	// param := [prop x, diff x, int x, prop y, diff y, int y, prop r, diff r, int r]
	
	// reduced parameters
	// assume: x and y are linearly related
	// no integral needed
	// params = 5
	// param := [prop x, diff x, y/x ratio, prop r, diff r]

	// input = 9
	// input := [dis x, vel x, int x, dis y, vel y, int y, dis r, vel r, int r]
	// output = 3
	// output := [vel x, vel y, vel r]

	class tunable_pid_controller_factory : public robot_controller_factory {
		public:
			tunable_pid_controller_factory() : robot_controller_factory("Tunable PID") {
			}

			robot_controller::ptr create_controller(player_impl::ptr plr, bool, unsigned int) const {
				robot_controller::ptr p(new tunable_pid_controller(plr));
				return p;
			}
	};

	tunable_pid_controller_factory factory;

	// parameters
#ifdef REDUCED_PARAMS
	const int P = 5;
	const double arr_min[P] = {0.0, 0.0, 0.8, 0.0, 0.0};
	const double arr_max[P] = {5.0, 0.8, 1.2, 3.0, 0.8};
	const double arr_def[P] = {5.0, 0.3, 1.0, 3.0, 0.3};
#else
	const int P = 9;
	const double arr_min[P] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	const double arr_max[P] = {5.0, 0.8, 0.1, 5.0, 0.8, 0.1, 3.0, 0.8, 0.1};
	const double arr_def[P] = {5.0, 0.3, 0.1, 5.0, 0.3, 0.1, 3.0, 0.3, 0.1};
#endif
}

const std::vector<double> tunable_pid_controller::param_min(arr_min, arr_min + P);
const std::vector<double> tunable_pid_controller::param_max(arr_max, arr_max + P);
const std::vector<double> tunable_pid_controller::param_default(arr_def, arr_def + P);

tunable_pid_controller::tunable_pid_controller(player_impl::ptr plr) : plr(plr), initialized(false), error_pos(10), error_ori(10) {
	// param := [prop x, diff x, int x, prop y, diff y, int y, prop r, diff r, int r]
	param = param_default;
}

void tunable_pid_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();

	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	const point &new_dir = (new_position - current_position).rotate(-current_orientation);

	if (new_da > PI) new_da -= 2 * PI;

	std::vector<double> input(9), out(3);

	if (!initialized) {
		initialized = true;
		// make error 0
		for (int t = 9; t > 0; --t) {
			error_pos[t] = new_dir;
			error_ori[t] = new_da;
		}
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

	// input := [dis x, vel x, int x, dis y, vel y, int y, dis r, vel r, int r]
	const double px = error_pos[0].x;
	const double vx = error_pos[0].x - error_pos[1].x;
	const double py = error_pos[0].y;
	const double vy = error_pos[0].y - error_pos[1].y;
	const double pa = error_ori[0];
	const double va = error_ori[0] - error_ori[1];

#ifdef REDUCED_PARAMS
	linear_velocity.x = px * param[0] + vx * param[1];
	linear_velocity.y = py * param[0] * param[2] + vy * param[1] * param[2];
	angular_velocity  = pa * param[3] + va * param[4];
#else
	const double cx = accum_pos.x;
	const double cy = accum_pos.y;
	const double ca = accum_ori;

	linear_velocity.x = px * param[0] + vx * param[1] + cx * param[2];
	linear_velocity.y = py * param[3] + vy * param[4] + cy * param[5];
	angular_velocity  = pa * param[6] + va * param[7] + ca * param[8];
#endif
}

robot_controller_factory &tunable_pid_controller::get_factory() const {
	return factory;
}

