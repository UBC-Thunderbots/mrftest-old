#ifndef AI_ROBOT_CONTROLLER_LAZY_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_LAZY_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "ai/robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

class LazyController : public RobotController {
	public:

		void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

		void clear();

		RobotControllerFactory &get_factory() const;

		LazyController(Player::Ptr plr);

	protected:
	private:
		Player::Ptr plr;
};

#endif

