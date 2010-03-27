#include "tunable_pid_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	// params = 9
	// param := [prop x, diff x, int x, prop y, diff y, int y, prop r, diff r, int r]
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

	// score as function of input
	// least squares error
	double score(const double inp[]) {
		// input := [dis x, vel x, int x, dis y, vel y, int y, dis r, vel r, int r]
		return 1e0*inp[0]*inp[0] + 1e0*inp[3]*inp[3] + 1e-2*inp[6]*inp[6] // displacement
			+ 1e-2*inp[1]*inp[1] + 1e-2*inp[4]*inp[4]; // velocity
	}

	// apply param to the inputs
	void apply(const std::vector<double>& param, const std::vector<double>& inp, std::vector<double>& out) {
		for(int i = 0; i < 3; ++i) {
			out[i] = 0;
			for(int j = 0; j < 3; ++j) {
				out[i] += param[3*i+j]*inp[3*i+j];
			}
		}
	}

	// learn rate
	const double learn = 1e-2;
}

std::vector<double> tunable_pid_controller::param_min;
std::vector<double> tunable_pid_controller::param_max;

tunable_pid_controller::tunable_pid_controller(player_impl::ptr plr) : plr(plr), initialized(false), param(9), error_pos(10), error_ori(10) {
	// param := [prop x, diff x, int x, prop y, diff y, int y, prop r, diff r, int r]
	param[0] = 4.0;
	param[1] = 0.5;
	param[2] = 0.0;
	param[3] = 4.0;
	param[4] = 0.5;
	param[5] = 0.0;
	param[6] = 3.0;
	param[7] = 0.01;
	param[8] = 0.0;

	if(param_min.size() == 0) {
		param_min.resize(9, 0.0);
		param_max.resize(9, 0.8);
		param_max[0] = 8;
		param_max[3] = 8;
		param_max[6] = 8;
	}
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
	input[0] = error_pos[0].x;
	input[1] = error_pos[0].x - error_pos[1].x;
	input[2] = accum_pos.x;
	input[3] = error_pos[0].y;
	input[4] = error_pos[0].y - error_pos[1].y;
	input[5] = accum_pos.y;
	input[6] = error_ori[0];
	input[7] = error_ori[0] - error_ori[1];
	input[8] = accum_ori;

	std::vector<double> output(3);
	apply(param, input, output);

	// output := [vel x, vel y, vel r]
	linear_velocity.x = output[0];
	linear_velocity.y = output[1];
	angular_velocity = output[2];
}

robot_controller_factory &tunable_pid_controller::get_factory() const {
	return factory;
}

