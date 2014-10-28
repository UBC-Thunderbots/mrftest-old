#include "ai/robot_controller/robot_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include "util/param.h"
#include <cmath>

using AI::RC::RobotController;
using namespace AI::RC::W;

namespace {
	class LazyController final : public RobotController {
		public:
			explicit LazyController(World world, Player player) : RobotController(world, player) {
			}

			void tick() override {
				int wheel_speeds[4] = { 0, 0, 0, 0 };
				player.drive(wheel_speeds);
			}
	};
}

ROBOT_CONTROLLER_REGISTER(LazyController)

