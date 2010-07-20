#ifndef ROBOT_CONTROLLER_MAX_POWER_CONTROLLER_H
#define ROBOT_CONTROLLER_MAX_POWER_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/memory.h"
#include "util/noncopyable.h"

class MaxPowerController : public RobotController {
	public:

		void move(const Point &new_position, double new_orientation, Point &lienar_velocity, double &angular_velocity);

		void clear();

		RobotControllerFactory &get_factory() const;

		MaxPowerController(RefPtr<Player> plr);

	protected:
	private:
		RefPtr<Player> plr;
};

#endif

