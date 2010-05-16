#ifndef ROBOT_CONTROLLER_LAZY_CONTROLLER_H
#define ROBOT_CONTROLLER_LAZY_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

class lazy_controller : public robot_controller {
	public:

		void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);

		void clear();

		robot_controller_factory &get_factory() const;

		lazy_controller(player::ptr plr);

	protected:
	private:
		player::ptr plr;
};

#endif

