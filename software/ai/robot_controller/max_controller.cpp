#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include <cmath>
#include <vector>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	class MaxController : public OldRobotController {
		public:
			void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity);
			void clear();
			RobotControllerFactory &get_factory() const;
			MaxController(World &world, Player::Ptr plr);
	};

	MaxController::MaxController(World &world, Player::Ptr plr) : OldRobotController(world, plr) {
	}

	void MaxController::move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) {
		const Angle current_orientation = player->orientation();
		angular_velocity = (new_orientation - current_orientation).angle_mod();
		linear_velocity = (new_position - player->position()).rotate(-current_orientation);
		if (linear_velocity.len() != 0) {
			linear_velocity = linear_velocity / linear_velocity.len() * 9001; // It's over NINE THOUSAAAAAND!!!
		}
	}

	void MaxController::clear() {
	}
}

ROBOT_CONTROLLER_REGISTER(MaxController)

