#ifndef AI_ROBOT_CONTROLLER_MAX_POWER_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_MAX_POWER_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "ai/robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

namespace AI {
	namespace RobotController {
		class MaxPowerController : public RobotController {
			public:

				void move(const Point &new_position, double new_orientation, Point &lienar_velocity, double &angular_velocity);

				void clear();

				RobotControllerFactory &get_factory() const;

				MaxPowerController(Player::Ptr plr);

			protected:
			private:
				Player::Ptr plr;
		};
	}
}

#endif

