#ifndef ROBOT_CONTROLLER_fuzzy_CONTROLLER_H
#define ROBOT_CONTROLLER_fuzzy_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

class fuzzy_controller : public virtual robot_controller {
	public:

		void move(const point &current_position, const point &new_position, double current_orientation, double new_orientation, point &linear_velocity, double &angular_velocity);

		robot_controller_factory &get_factory() const;

		fuzzy_controller();

	protected:
};

#endif

