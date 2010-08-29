#ifndef AI_ROBOT_CONTROLLER_LINEAR_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_LINEAR_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "ai/robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

namespace AI {
	namespace RobotController {
		class LinearController : public RobotController {
			public:

				void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

				void clear();

				RobotControllerFactory &get_factory() const;

				LinearController(Player::Ptr plr);

			protected:
			private:
				Player::Ptr plr;
		};
	}
}

#endif

