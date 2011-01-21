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
	BoolParam ADHOC_SLOW_ANGULAR("AdHoc2: Slow if translating", true);
	BoolParam ADHOC_FLIP_SLOWDOWN("AdHoc2: flip trans/ang slowdown", false);
	DoubleParam ADHOC_SLOWDOWN("AdHoc2: slowdown (CARE)", 1.5, 0.1, 8.0);
	DoubleParam ADHOC_PROP("AdHoc2: prop", 8, 0.0, 20.0);
	DoubleParam ADHOC_MAX_VEL("AdHoc2: max vel", 6, 0.0, 20.0);
	DoubleParam ADHOC_MAX_ACC("AdHoc2: max acc", 3, 0.0, 20.0);
	DoubleParam ADHOC_A_PROP("AdHoc2: angle prop", 12, 0.0, 50.0);
	DoubleParam ADHOC_A_THRESH("AdHoc2: angle thresh", 12, 0.0, 50.0);

	const double ADHOC_XY_RATIO = 0.81;
	const double ADHOC_DIFF = 0.0;
	const double ADHOC_A_DIFF = 0.0;
	const double ADHOC_YA_RATIO = 0.0; // 0 - 5 to face forwards

	class AdHoc2Controller : public OldRobotController {
		public:
			void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);
			void clear();
			RobotControllerFactory &get_factory() const;
			AdHoc2Controller(World &world, Player::Ptr plr);

		protected:
			bool initialized;
			// errors in x, y, d
			Point prev_new_pos;
			double prev_new_ori;
			Point prev_linear_velocity;
			double prev_angular_velocity;
	};

	AdHoc2Controller::AdHoc2Controller(World &world, Player::Ptr plr) : OldRobotController(world, plr), initialized(false), prev_linear_velocity(0.0, 0.0), prev_angular_velocity(0.0) {
	}

	void AdHoc2Controller::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
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

		linear_velocity.x = px * ADHOC_PROP + vx * ADHOC_DIFF;
		linear_velocity.y = (py * ADHOC_PROP + vy * ADHOC_DIFF) * ADHOC_XY_RATIO;

		// threshold the linear velocity
		if (linear_velocity.len() > ADHOC_MAX_VEL) {
			linear_velocity *= ADHOC_MAX_VEL / linear_velocity.len();
		}

		// threshold the linear acceleration
		Point accel = linear_velocity - prev_linear_velocity;
		if (accel.len() > ADHOC_MAX_ACC) {
			accel *= ADHOC_MAX_ACC / accel.len();
			linear_velocity = prev_linear_velocity + accel;
		}

		angular_velocity = pa * ADHOC_A_PROP + va * ADHOC_A_DIFF + linear_velocity.y * ADHOC_YA_RATIO;
		angular_velocity = clamp<double>(angular_velocity, -ADHOC_A_THRESH, ADHOC_A_THRESH);

		// threshold even more
		if (ADHOC_SLOW_ANGULAR) {
			if (ADHOC_FLIP_SLOWDOWN) {
				double slowdown = (ADHOC_SLOWDOWN * ADHOC_A_THRESH - std::fabs(angular_velocity)) / (ADHOC_SLOWDOWN * ADHOC_A_THRESH);
				if (std::fabs(slowdown) > 1.1) {
					std::cerr << "adhoc2: spin up" << std::endl;
					slowdown = 1;
				}
				linear_velocity *= slowdown;
			} else {
				double slowdown = (ADHOC_SLOWDOWN * ADHOC_MAX_VEL - linear_velocity.len()) / (ADHOC_SLOWDOWN * ADHOC_MAX_VEL);
				if (std::fabs(slowdown) > 1.1) {
					std::cerr << "adhoc2: spin up" << std::endl;
					slowdown = 1;
				}
				angular_velocity *= slowdown;
			}
		}

		prev_linear_velocity = linear_velocity;
		prev_angular_velocity = angular_velocity;
	}

	void AdHoc2Controller::clear() {
	}

	class AdHoc2ControllerFactory : public RobotControllerFactory {
		public:
			AdHoc2ControllerFactory() : RobotControllerFactory("adhoc2") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr plr) const {
				RobotController::Ptr p(new AdHoc2Controller(world, plr));
				return p;
			}
	};

	AdHoc2ControllerFactory factory;

	RobotControllerFactory &AdHoc2Controller::get_factory() const {
		return factory;
	}
}

