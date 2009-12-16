#ifndef ROBOT_CONTROLLER_PID_CONTROLLER_H
#define ROBOT_CONTROLLER_PID_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "world/player_impl.h"

class jons_controller : public robot_controller {
	public:

		void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);

		robot_controller_factory &get_factory() const;

		jons_controller(player_impl::ptr plr);
	private:
		player_impl::ptr plr;
	
	protected:
		double max_acc;
		double max_vel;
		double max_Aacc;
		double close_param;
		double position_delta;
		double orient_delta;

};

#endif

