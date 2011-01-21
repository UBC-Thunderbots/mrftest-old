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
	class MaxController : public OldRobotController {
		public:
			void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);
			void clear();
			RobotControllerFactory &get_factory() const;
			MaxController(World &world, Player::Ptr plr);
	};

	MaxController::MaxController(World &world, Player::Ptr plr) : OldRobotController(world, plr) {
	}

	void MaxController::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
		const double current_orientation = player->orientation();
		angular_velocity = angle_mod(new_orientation - current_orientation);
		linear_velocity = (new_position - player->position()).rotate(-current_orientation);
		if (linear_velocity.len() != 0) {
			linear_velocity = linear_velocity / linear_velocity.len() * 9001; // It's over NINE THOUSAAAAAND!!!
		}
	}

	void MaxController::clear() {
	}

	class MaxControllerFactory : public RobotControllerFactory {
		public:
			MaxControllerFactory() : RobotControllerFactory("MAX") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr plr) const {
				RobotController::Ptr p(new MaxController(world, plr));
				return p;
			}
	};

	MaxControllerFactory factory;

	RobotControllerFactory &MaxController::get_factory() const {
		return factory;
	}
}

