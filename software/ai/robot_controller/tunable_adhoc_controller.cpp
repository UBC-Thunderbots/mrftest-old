#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "uicomponents/param.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <cassert>
#include <cmath>
#include <glibmm.h>
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

	class TunableAdHocController : public OldRobotController, public TunableController {
		public:
			void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);
			void clear();
			RobotControllerFactory &get_factory() const;
			TunableAdHocController(Player::Ptr plr);
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
			std::vector<double> error_ori;
			Point prev_new_pos;
			double prev_new_ori;
			Point prev_linear_velocity;
			double prev_angular_velocity;
	};

	TunableAdHocController::TunableAdHocController(Player::Ptr plr) : OldRobotController(plr), initialized(false), error_pos(10.0), error_ori(10.0), prev_linear_velocity(0.0, 0.0), prev_angular_velocity(0.0) {
		param = param_default;
	}

	const std::vector<std::string> TunableAdHocController::get_params_name() const {
		return std::vector<std::string>(PARAM_NAMES, PARAM_NAMES + P);
	}

	void TunableAdHocController::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
		const Point &current_position = player->position();
		const double current_orientation = player->orientation();

		// relative new direction and angle
		double new_da = angle_mod(new_orientation - current_orientation);
		const Point &new_dir = (new_position - current_position).rotate(-current_orientation);

		if (new_da > M_PI) {
			new_da -= 2 * M_PI;
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
		const double pa = new_da;
		Point vel = (player->velocity()).rotate(-current_orientation);
		double vx = -vel.x;
		double vy = -vel.y;
		double va = -player->avelocity();

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

		angular_velocity = pa * param[PARAM_A_PROP] + va * param[PARAM_A_DIFF] + linear_velocity.y * param[PARAM_YA_RATIO];

		// threshold the angular velocity
		if (angular_velocity > param[PARAM_A_THRESH]) {
			angular_velocity = param[PARAM_A_THRESH];
		} else if (angular_velocity < -param[PARAM_A_THRESH]) {
			angular_velocity = -param[PARAM_A_THRESH];
		}

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

	void TunableAdHocController::clear() {
#warning WRITE CODE HERE
	}

	class TunableAdHocControllerFactory : public RobotControllerFactory {
		public:
			TunableAdHocControllerFactory() : RobotControllerFactory("Ad Hoc =D") {
			}

			RobotController::Ptr create_controller(Player::Ptr plr) const {
				RobotController::Ptr p(new TunableAdHocController(plr));
				return p;
			}
	};

	TunableAdHocControllerFactory factory;

	RobotControllerFactory &TunableAdHocController::get_factory() const {
		return factory;
	}
}

