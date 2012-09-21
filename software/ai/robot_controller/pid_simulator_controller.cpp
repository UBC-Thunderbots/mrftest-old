#include "ai/robot_controller/robot_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include <cmath>
#include <vector>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	BoolParam PID_SLOW_ANGULAR("(CARE) reduce angular velocity when translating", "RC/PIDSim", false);
	BoolParam PID_FLIP_SLOWDOWN("(CARE) flip trans&ang slowdown", "RC/PIDSim", false);
	DoubleParam PID_SLOWDOWN("(CARE) angular slowdown when translating", "RC/PIDSim", 1.01, 0.1, 8.0);
	DoubleParam PID_PROP("xy +proportional", "RC/PIDSim", 0.1, 0.0, 50.0);
	DoubleParam PID_DIFF("xy -differential", "RC/PIDSim", 0, -50.0, 50.0);
	DoubleParam PID_INTG("xy +integral", "RC/PIDSim", 0, 0.0, 10.0);
	DoubleParam PID_MAX_VEL("xy max velocity", "RC/PIDSim", 1, 0.0, 50.0);
	DoubleParam PID_MAX_ACC("xy max acceleration", "RC/PIDSim", 1, 0.0, 50.0);
	DoubleParam PID_A_PROP("angular +proportional", "RC/PIDSim", 1, 0.0, 50.0);
	DoubleParam PID_A_DIFF("angular -differential", "RC/PIDSim", 0, -50.0, 50.0);
	DoubleParam PID_A_INTG("angular +integral", "RC/PIDSim", 0, 0.0, 10.0);
	RadianParam PID_A_THRESH("angular max velocity (radians)", "RC/PIDSim", 1, 0.0, 50.0);
	DoubleParam PID_XY_RATIO("x to y ratio", "RC/PIDSim", 1, 0.0, 2.0);
	DoubleParam PID_DAMP("integral decay (%)", "RC/PIDSim", 0, 0, 99.0);

	DoubleParam PID_YA_RATIO("YA ratio", "RC/PIDSim", 0, 0, 10.0);

	BoolParam DO_RESET("do reset when target changes", "RC/PIDSim", false);

	class PIDSimController : public OldRobotController {
		public:
			void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity);
			void clear();
			explicit PIDSimController(World world, Player::Ptr plr);

		protected:
			bool initialized;
			// errors in x, y, d
			Point prev_new_pos;
			Angle prev_new_ori;
			Point prev_linear_velocity;
			Angle prev_angular_velocity;

			Point integral_xy;
			Angle integral_a;
	};

	PIDSimController::PIDSimController(World world, Player::Ptr plr) : OldRobotController(world, plr), initialized(false), prev_linear_velocity(0.0, 0.0), prev_angular_velocity(Angle::ZERO), integral_a(Angle::ZERO) {
	}

	void PIDSimController::move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) {
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
			prev_new_pos = new_position;
			prev_new_ori = new_orientation;
		}

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
		if (DO_RESET && (prev_new_pos.x != new_position.x || prev_new_pos.y != new_position.y || prev_new_ori != new_orientation)) {
			prev_new_pos = new_position;
			prev_new_ori = new_orientation;
			integral_xy = Point(0.0, 0.0);
			integral_a = Angle::ZERO;
		} else {
			integral_xy *= PID_DAMP / 100.0;
			integral_xy += new_dir;
			integral_a *= PID_DAMP / 100.0;
			integral_a += new_da;
		}

		linear_velocity.x = px * PID_PROP + vx * PID_DIFF + integral_xy.x * PID_INTG;
		linear_velocity.y = (py * PID_PROP + vy * PID_DIFF + integral_xy.y * PID_INTG) * PID_XY_RATIO;

		// threshold the linear velocity
		if (linear_velocity.len() > PID_MAX_VEL) {
			linear_velocity *= PID_MAX_VEL / linear_velocity.len();
		}

		// threshold the linear acceleration
		Point accel = linear_velocity - prev_linear_velocity;
		if (accel.len() > PID_MAX_ACC) {
			accel *= PID_MAX_ACC / accel.len();
			linear_velocity = prev_linear_velocity + accel;
		}

		angular_velocity = pa * PID_A_PROP + va * PID_A_DIFF + Angle::of_radians(linear_velocity.y * PID_YA_RATIO) + integral_a * PID_A_INTG;
		angular_velocity = clamp_symmetric(angular_velocity, PID_A_THRESH.get());

		// threshold even more
		if (PID_SLOW_ANGULAR) {
			if (PID_FLIP_SLOWDOWN) {
				double slowdown = (PID_SLOWDOWN * PID_A_THRESH - angular_velocity.abs()) / (PID_SLOWDOWN * PID_A_THRESH);
				if (std::fabs(slowdown) > 1.1) {
					LOG_ERROR("spin up");
					slowdown = 1;
				}
				linear_velocity *= slowdown;
			} else {
				double slowdown = (PID_SLOWDOWN * PID_MAX_VEL - linear_velocity.len()) / (PID_SLOWDOWN * PID_MAX_VEL);
				if (std::fabs(slowdown) > 1.1) {
					LOG_ERROR("spin up");
					slowdown = 1;
				}
				angular_velocity *= slowdown;
			}
		}

		prev_linear_velocity = linear_velocity;
		prev_angular_velocity = angular_velocity;
	}

	void PIDSimController::clear() {
	}
}

ROBOT_CONTROLLER_REGISTER(PIDSimController)

