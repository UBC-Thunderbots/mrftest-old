#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <cmath>
#include <glibmm.h>
#include <iostream>
#include <vector>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	BoolParam PID_SLOW_ANGULAR("PID3: Slow if translating", true);
	BoolParam PID_FLIP_SLOWDOWN("PID3: flip trans/ang slowdown", false);
	DoubleParam PID_SLOWDOWN("PID3: slowdown (CARE)", 1.5, 0.1, 8.0);
	DoubleParam PID_PROP("PID3: prop", 20, 0.0, 20.0);
	DoubleParam PID_DIFF("PID3: diff", 4, 0.0, 20.0);
	DoubleParam PID_MAX_VEL("PID3: max vel", 6, 0.0, 20.0);
	DoubleParam PID_MAX_ACC("PID3: max acc", 3, 0.0, 20.0);
	DoubleParam PID_A_PROP("PID3: angle prop", 18, 0.0, 50.0);
	DoubleParam PID_A_DIFF("PID3: angle diff", 2, 0.0, 20.0);
	DoubleParam PID_A_THRESH("PID3: angle thresh", 12, 0.0, 50.0);
	DoubleParam PID_XY_RATIO("PID3: xy ratio", 0.81, 0.0, 2.0);

	const double PID_YA_RATIO = 0.0; // 0 - 5 to face forwards

	class PID3Controller : public OldRobotController {
		public:
			void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);
			void clear();
			RobotControllerFactory &get_factory() const;
			PID3Controller(World &world, Player::Ptr plr);

		protected:
			bool initialized;
			// errors in x, y, d
			Point prev_new_pos;
			double prev_new_ori;
			Point prev_linear_velocity;
			double prev_angular_velocity;
	};

	PID3Controller::PID3Controller(World &world, Player::Ptr plr) : OldRobotController(world, plr), initialized(false), prev_linear_velocity(0.0, 0.0), prev_angular_velocity(0.0) {
	}

	void PID3Controller::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
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
			prev_new_pos = new_position;
			prev_new_ori = new_orientation;
		}

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

		linear_velocity.x = px * PID_PROP + vx * PID_DIFF;
		linear_velocity.y = (py * PID_PROP + vy * PID_DIFF) * PID_XY_RATIO;

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

		angular_velocity = pa * PID_A_PROP + va * PID_A_DIFF + linear_velocity.y * PID_YA_RATIO;
		angular_velocity = clamp<double>(angular_velocity, -PID_A_THRESH, PID_A_THRESH);

		// threshold even more
		if (PID_SLOW_ANGULAR) {
			if (PID_FLIP_SLOWDOWN) {
				double slowdown = (PID_SLOWDOWN * PID_A_THRESH - std::fabs(angular_velocity)) / (PID_SLOWDOWN * PID_A_THRESH);
				if (std::fabs(slowdown) > 1.1) {
					std::cerr << "PID3: spin up" << std::endl;
					slowdown = 1;
				}
				linear_velocity *= slowdown;
			} else {
				double slowdown = (PID_SLOWDOWN * PID_MAX_VEL - linear_velocity.len()) / (PID_SLOWDOWN * PID_MAX_VEL);
				if (std::fabs(slowdown) > 1.1) {
					std::cerr << "PID3: spin up" << std::endl;
					slowdown = 1;
				}
				angular_velocity *= slowdown;
			}
		}

		prev_linear_velocity = linear_velocity;
		prev_angular_velocity = angular_velocity;
	}

	void PID3Controller::clear() {
	}

	class PID3ControllerFactory : public RobotControllerFactory {
		public:
			PID3ControllerFactory() : RobotControllerFactory("PID 3") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr plr) const {
				RobotController::Ptr p(new PID3Controller(world, plr));
				return p;
			}
	};

	PID3ControllerFactory factory;

	RobotControllerFactory &PID3Controller::get_factory() const {
		return factory;
	}
}

