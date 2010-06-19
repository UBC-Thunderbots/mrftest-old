#include "geom/point.h"
#include "geom/angle.h"
#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "robot_controller/tunable_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

#include <vector>
#include <glibmm.h>
#include <cmath>
#include <iostream>
#include <fstream>

namespace {

	const double DAMP = 0.5;

	const std::string PARAM_NAMES[] = {"Proportional", "Differential", "Y/X Ratio", "Maximum Speed", "Maximum Acceleration", "Proportional Angle", "Differential Angle", "Maximum Angular Speed", "Y/Angle speed ratio compensate"};

	// enumerate the parameters
	enum { PARAM_PROP = 0, PARAM_DIFF, PARAM_XY_RATIO, PARAM_MAX_VEL, PARAM_MAX_ACC, PARAM_A_PROP, PARAM_A_DIFF, PARAM_A_THRESH, PARAM_YA_RATIO };

	const double DEF_PROP = 8.0;
	const double DEF_DIFF = 0.0;
	const double DEF_XY_RATIO = 0.81;
	const double DEF_MAX_VEL = 8.0; // 3.0 is stable, 3.5 is slightly unstable
	const double DEF_MAX_ACC = 3.0;
	const double DEF_A_PROP = 20.0;
	const double DEF_A_DIFF = 0.0;
	const double DEF_A_THRESH = 10.0;
	const double DEF_YA_RATIO = 10.0; // make it face forwards

#warning put this magic number as part of the tunable parameter
	const double DEF_HAS_BALL_RATIO = 0.8;

	// array of defaults
	const double ARR_DEF[] = { DEF_PROP, DEF_DIFF, DEF_XY_RATIO, DEF_MAX_VEL, DEF_MAX_ACC, DEF_A_PROP, DEF_A_DIFF, DEF_A_THRESH, DEF_YA_RATIO };
	const int P = sizeof(ARR_DEF) / sizeof(ARR_DEF[0]);

	class tunable_adhoc_controller : public robot_controller, public tunable_controller {
		public:
			void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);
			void clear();
			robot_controller_factory &get_factory() const;
			tunable_adhoc_controller(player::ptr plr);
			void set_params(const std::vector<double>& params) {
				this->param = params;
			}
			const std::vector<std::string> get_params_name() const;
			const std::vector<double>& get_params() const {
				return param;
			}
		private:
			player::ptr plr;
		protected:
			bool initialized;
			std::vector<double> param;
			// errors in x, y, d
			std::vector<point> error_pos;
			std::vector<double> error_ori;
			point prev_new_pos;
			double prev_new_ori;
			point prev_linear_velocity;
			double prev_angular_velocity;
	};

	const std::vector<double> param_default(ARR_DEF, ARR_DEF + P);

	tunable_adhoc_controller::tunable_adhoc_controller(player::ptr plr) : plr(plr), initialized(false), error_pos(10.0), error_ori(10.0), prev_linear_velocity(0.0, 0.0), prev_angular_velocity(0.0) {
		param = param_default;
	}

	const std::vector<std::string> tunable_adhoc_controller::get_params_name() const {
		return std::vector<std::string>(PARAM_NAMES, PARAM_NAMES + P);
	}

	void tunable_adhoc_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
		const point &current_position = plr->position();
		const double current_orientation = plr->orientation();

		// relative new direction and angle
		double new_da = angle_mod(new_orientation - current_orientation);
		const point &new_dir = (new_position - current_position).rotate(-current_orientation);

		if (new_da > M_PI) new_da -= 2 * M_PI;

		if (!initialized) {
			initialized = true;
			// make error 0
			for (int t = 9; t > 0; --t) {
				error_pos[t] = new_dir;
				error_ori[t] = new_da;
			}
			prev_new_pos = new_position;
			prev_new_ori = new_orientation;
		}

		// update the previous
		/*
		for (int t = 9; t > 0; --t) {
			error_pos[t] = error_pos[t - 1];
			error_ori[t] = error_ori[t - 1];
		}
		error_pos[0] = new_dir;
		error_ori[0] = new_da;
		*/

		/*
		point accum_pos(0, 0);
		double accum_ori(0);
		for (int t = 9; t >= 0; --t) {
			accum_pos *= DAMP;
			accum_ori *= DAMP;
			accum_pos += error_pos[t];
			accum_ori += error_ori[t];
		}
		*/

		const double px = new_dir.x;
		const double py = new_dir.y;
		const double pa = new_da;
		point vel = (plr->est_velocity()).rotate(-current_orientation);
		double vx = -vel.x;
		double vy = -vel.y;
		double va = -plr->est_avelocity();

		//const double cx = accum_pos.x;
		//const double cy = accum_pos.y;

		// check if command has changed
		if (prev_new_pos.x != new_position.x || prev_new_pos.y != new_position.y || prev_new_ori != new_orientation) {
			prev_new_pos = new_position;
			prev_new_ori = new_orientation;
		}

		linear_velocity.x = px * param[PARAM_PROP] + vx * param[PARAM_DIFF];
		linear_velocity.y = (py * param[PARAM_PROP] + vy * param[PARAM_DIFF]) * param[PARAM_XY_RATIO];

		// threshold the linear velocity
		if (linear_velocity.len() > param[PARAM_MAX_VEL]) {
			linear_velocity *= param[PARAM_MAX_VEL] / linear_velocity.len();
		}

		// threshold the linear acceleration
		point accel = linear_velocity - prev_linear_velocity;
		if (accel.len() > param[PARAM_MAX_ACC]) {
			accel *= param[PARAM_MAX_ACC] / accel.len();
			linear_velocity = prev_linear_velocity + accel;
		}

		angular_velocity = pa * param[PARAM_A_PROP] + va * param[PARAM_A_DIFF] + linear_velocity.y * param[PARAM_YA_RATIO];

		// threshold the angular velocity
		if (angular_velocity > param[PARAM_A_THRESH]) {
			angular_velocity = param[PARAM_A_THRESH];
		} else if (angular_velocity < -param[PARAM_A_THRESH]) {
			angular_velocity = -param[PARAM_A_THRESH];
		}

		/*
		if (plr->has_ball()) {
			angular_velocity *= DEF_HAS_BALL_RATIO;
			linear_velocity *= DEF_HAS_BALL_RATIO;
		}
		*/

		prev_linear_velocity = linear_velocity;
		prev_angular_velocity = angular_velocity;
	}

	void tunable_adhoc_controller::clear() {
#warning WRITE CODE HERE
	}

	class tunable_adhoc_controller_factory : public robot_controller_factory {
		public:
			tunable_adhoc_controller_factory() : robot_controller_factory("Ad Hoc =D") {
			}

			robot_controller::ptr create_controller(player::ptr plr, bool, unsigned int) const {
				robot_controller::ptr p(new tunable_adhoc_controller(plr));
				return p;
			}
	};

	tunable_adhoc_controller_factory factory;

	robot_controller_factory &tunable_adhoc_controller::get_factory() const {
		return factory;
	}

}

