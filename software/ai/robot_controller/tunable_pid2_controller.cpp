#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include <cassert>
#include <cmath>
#include <vector>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using AI::RC::TunableController;
using namespace AI::RC::W;

namespace {
	// BoolParam PID_SLOW_ANGULAR("PID: Slow if translating", false);
	// BoolParam PID_FLIP_SLOWDOWN("PID: flip trans/ang slowdown", false);

	const double SLOWDOWN = 2.0;
	const double DAMP = 0.5;

	const std::string PARAM_NAMES[] = { "Proportional", "Differential", "Y/X Ratio", "Maximum Speed", "Maximum Acceleration", "Proportional Angle", "Differential Angle", "Maximum Angular Speed", "Y/Angle speed ratio compensate" };

	// enumerate the parameters
	enum {
		PARAM_PROP = 0, PARAM_DIFF, PARAM_XY_RATIO, PARAM_MAX_VEL, PARAM_MAX_ACC, PARAM_A_PROP, PARAM_A_DIFF, PARAM_A_THRESH, PARAM_YA_RATIO
	};

	const double DEF_PROP = 8.0; // 8 - 10
	const double DEF_DIFF = 0.0;
	const double DEF_XY_RATIO = 0.81;
	const double DEF_MAX_VEL = 6.0; // 4 - 6
	const double DEF_MAX_ACC = 3.0; // 1 - 3
	const double DEF_A_PROP = 8.0; // 10 - ?
	const double DEF_A_DIFF = 0.0;
	const double DEF_A_THRESH = 8.0; // 8 - ?
	const double DEF_YA_RATIO = 5.0; // 0 - 5 to face forwards

#warning put this magic number as part of the tunable parameter
	const double DEF_HAS_BALL_RATIO = 0.8;

	// array of defaults
	const double ARR_DEF[] = { DEF_PROP, DEF_DIFF, DEF_XY_RATIO, DEF_MAX_VEL, DEF_MAX_ACC, DEF_A_PROP, DEF_A_DIFF, DEF_A_THRESH, DEF_YA_RATIO };
	const int P = sizeof(ARR_DEF) / sizeof(ARR_DEF[0]);
	const std::vector<double> param_default(ARR_DEF, ARR_DEF + P);

	class TunablePID2Controller : public OldRobotController, public TunableController {
		public:
			void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity);
			void clear();
			TunablePID2Controller(World &world, Player::Ptr plr);
			void set_params(const std::vector<double> &params) {
				this->param = params;
			}
			const std::vector<std::string> get_params_name() const;
			const std::vector<double> get_params() const {
				return param;
			}
			const std::vector<double> get_params_default() const {
				return param_default;
			}

		protected:
			bool initialized;
			std::vector<double> param;
			// errors in x, y, d
			std::vector<Point> error_pos;
			std::vector<Angle> error_ori;
			Point prev_new_pos;
			Angle prev_new_ori;
			Point prev_linear_velocity;
			Angle prev_angular_velocity;
	};

	TunablePID2Controller::TunablePID2Controller(World &world, Player::Ptr plr) : OldRobotController(world, plr), initialized(false), param(param_default), error_pos(10.0), error_ori(10.0), prev_linear_velocity(0.0, 0.0), prev_angular_velocity(Angle::ZERO) {
	}

	const std::vector<std::string> TunablePID2Controller::get_params_name() const {
		return std::vector<std::string>(PARAM_NAMES, PARAM_NAMES + P);
	}

	void TunablePID2Controller::move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) {
		const Point &current_position = player->position();
		const Angle current_orientation = player->orientation();

		// relative new direction and angle
		Angle new_da = (new_orientation - current_orientation).angle_mod();
		const Point &new_dir = (new_position - current_position).rotate(-current_orientation);

		if (new_da > Angle::HALF) {
			new_da -= Angle::FULL;
		}

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
		   Point accum_pos(0, 0);
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
		const Angle pa = new_da;
		Point vel = (player->velocity()).rotate(-current_orientation);
		double vx = -vel.x;
		double vy = -vel.y;
		Angle va = -player->avelocity();

		// const double cx = accum_pos.x;
		// const double cy = accum_pos.y;

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
		Point accel = linear_velocity - prev_linear_velocity;
		if (accel.len() > param[PARAM_MAX_ACC]) {
			accel *= param[PARAM_MAX_ACC] / accel.len();
			linear_velocity = prev_linear_velocity + accel;
		}

		angular_velocity = pa * param[PARAM_A_PROP] + va * param[PARAM_A_DIFF] + Angle::of_radians(linear_velocity.y * param[PARAM_YA_RATIO]);

		// threshold the angular velocity
		angular_velocity = clamp_symmetric(angular_velocity, Angle::of_radians(param[PARAM_A_THRESH]));

		// threshold even more
		/*
		   if (PID_SLOW_ANGULAR) {
		    if (PID_FLIP_SLOWDOWN) {
		        double slowdown = (SLOWDOWN * param[PARAM_A_THRESH] - std::fabs(angular_velocity)) / (SLOWDOWN * param[PARAM_A_THRESH]);
		        assert(std::fabs(slowdown) < 1.1);
		        linear_velocity *= slowdown;
		    } else {
		        double slowdown = (SLOWDOWN * param[PARAM_MAX_VEL] - linear_velocity.len()) / (SLOWDOWN * param[PARAM_MAX_VEL]);
		        assert(std::fabs(slowdown) < 1.1);
		        angular_velocity *= slowdown;
		    }
		   }
		 */

		/*
		   if (player->has_ball()) {
		   angular_velocity *= DEF_HAS_BALL_RATIO;
		   linear_velocity *= DEF_HAS_BALL_RATIO;
		   }
		 */

		prev_linear_velocity = linear_velocity;
		prev_angular_velocity = angular_velocity;
	}

	void TunablePID2Controller::clear() {
#warning WRITE CODE HERE
	}
}

ROBOT_CONTROLLER_REGISTER(TunablePID2Controller)

