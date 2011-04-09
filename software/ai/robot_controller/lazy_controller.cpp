#include "ai/robot_controller/robot_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include <cmath>
#include <glibmm.h>
#include <map>
#include "uicomponents/param.h"

using AI::RC::RobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {

	class LazyController : public RobotController {
		public:
			LazyController(World &world, Player::Ptr player) : RobotController(world, player) {
			}

			void tick() {
				int wheel_speeds[4] = { 0, 0, 0, 0 };
				player->drive(wheel_speeds);
			}
	};

	class LazyControllerFactory : public RobotControllerFactory {
		public:
			LazyControllerFactory() : RobotControllerFactory("Lazy") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr player) const {
				RobotController::Ptr p(new LazyController(world, player));
				return p;
			}
	};

	LazyControllerFactory factory;
}

