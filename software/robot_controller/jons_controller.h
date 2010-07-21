#ifndef ROBOT_CONTROLLER_PID_CONTROLLER_H
#define ROBOT_CONTROLLER_PID_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "gpc.h"

/**
Controller to exploit old broken simulator
This class is more or less obsolete, however, current use could 
result in much needed hilarity with our current robots
*/
class JonsController : public RobotController {
	public:

		void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

		void clear();

		RobotControllerFactory &get_factory() const;

		JonsController(Player::Ptr plr);
	private:
		Player::Ptr plr;
	
	protected:
		GPC X_controller;
		GPC Y_controller;
		GPC T_controller;
		
		unsigned int learning_time;
		
		Point old_control;
		double old_ang;
		double max_acc;
		double max_vel;
		double max_Aacc;
		double close_param;
		double position_delta;
		double orient_delta;

};

#endif

