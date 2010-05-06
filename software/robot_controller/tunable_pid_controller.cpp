#include "tunable_pid_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

#define NO_ROTATION
#define LINEAR_XY

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

	// basic
	const double DEF_X_PROP = 6.95932;
	const double MIN_X_PROP = 0;
	const double MAX_X_PROP = 20;
	
	const double DEF_X_DIFF = 28.264;
	const double MIN_X_DIFF = 10;
	const double MAX_X_DIFF = 50;

#ifndef LINEAR_XY
	// if y independent of x

	const double DEF_Y_PROP = 7;
	const double MIN_Y_PROP = 0;
	const double MAX_Y_PROP = 20;

	const double DEF_Y_DIFF = 0;
	const double MIN_Y_DIFF = 10;
	const double MAX_Y_DIFF = 50;

#else
	// if x and y are tied linearly

	const double DEF_XY_RATIO = 1.0;
	const double MIN_XY_RATIO = 0.8;
	const double MAX_XY_RATIO = 1.2;

#endif

#ifndef NO_ROTATION

	// basic
	const double DEF_A_PROP = 7;
	const double MIN_A_PROP = 0;
	const double MAX_A_PROP = 20;
	
	const double DEF_A_DIFF = 0;
	const double MIN_A_DIFF = 10;
	const double MAX_A_DIFF = 50;

#endif

enum {
	PARAM_X_PROP = 0,
	PARAM_X_DIFF,
#ifndef LINEAR_XY
	PARAM_Y_PROP, PARAM_Y_DIFF,
#else
	PARAM_XY_RATIO,
#endif
#ifndef NO_ROTATION
	PARAM_A_PROP, PARAM_A_DIFF,
#endif
};

const double arr_min[] = {MIN_X_PROP, MIN_X_DIFF,
#ifndef LINEAR_XY
	MIN_Y_PROP, MIN_Y_DIFF,
#else
	MIN_XY_RATIO,
#endif
#ifndef NO_ROTATION
	MIN_A_PROP, MIN_A_DIFF,
#endif
};

const double arr_def[] = {DEF_X_PROP, DEF_X_DIFF,
#ifndef LINEAR_XY
	DEF_Y_PROP, DEF_Y_DIFF,
#else
	DEF_XY_RATIO,
#endif
#ifndef NO_ROTATION
	DEF_A_PROP, DEF_A_DIFF,
#endif
};

const double arr_max[] = {MAX_X_PROP, MAX_X_DIFF,
#ifndef LINEAR_XY
	MAX_Y_PROP, MAX_Y_DIFF,
#else
	MAX_XY_RATIO,
#endif
#ifndef NO_ROTATION
	MAX_A_PROP, MAX_A_DIFF,
#endif
};

const int P = sizeof(arr_max) / sizeof(arr_max[0]);

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

	linear_velocity.x = px * param[PARAM_X_PROP] + vx * param[PARAM_X_DIFF];

#ifndef LINEAR_XY
	//const double cx = accum_pos.x;
	//const double cy = accum_pos.y;
	linear_velocity.y = py * param[PARAM_Y_PROP] + vy * param[PARAM_Y_DIFF];
#else
	linear_velocity.y = (py * param[PARAM_X_PROP] + vy * param[PARAM_X_DIFF]) * param[PARAM_XY_RATIO];
#endif

#ifndef NO_ROTATION
	const double pa = error_ori[0];
	const double va = error_ori[0] - error_ori[1];
	// const double ca = accum_ori;
	angular_velocity = pa * param[PARAM_A_PROP] + va * param[PARAM_A_DIFF];
#else
	angular_velocity = 0;
#endif
}

void tunable_pid_controller::set_params(const std::vector<double>& params) {
	this->param = params;
}

const std::vector<double>& tunable_pid_controller::get_params() const {
	return param;
}

robot_controller_factory &tunable_pid_controller::get_factory() const {
	return factory;
}
