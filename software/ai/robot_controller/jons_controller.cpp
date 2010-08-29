#include "ai/robot_controller/jons_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "util/timestep.h"
#include <cmath>
#include <iostream>

using namespace AI::RobotController;

#warning this class needs Doxygen comments in its header file

namespace {

	class JonsControllerFactory : public RobotControllerFactory {
		public:
			JonsControllerFactory() : RobotControllerFactory("JONS RC") {
			}

			RobotController::Ptr create_controller(AI::Player::Ptr plr, bool, unsigned int) const {
				RobotController::Ptr p(new JonsController(plr));
				return p;
			}
	};

	JonsControllerFactory factory;

}



JonsController::JonsController(Player::Ptr plr) : plr(plr), max_acc(10), max_vel(1000), max_Aacc(1), close_param(1.5),position_delta(0.05), orient_delta(0.05)
{
	learning_time=0;
}

void JonsController::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
	
	const Point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	const Point &current_velocity = plr->est_velocity();
	const Point diff = new_position - current_position;
	Point new_linear_velocity;	
	double current_angularvel = plr->est_avelocity();
	
	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	
	angular_velocity = new_da;		
	
	Point new_dir = diff.rotate(-current_orientation);
	
	//X_controller.update(current_velocity.rotate(-current_orientation).x);
	//Y_controller.update(current_velocity.rotate(-current_orientation).y);
	//T_controller.update(current_angularvel);
	
	if(learning_time++ < 300) {
		linear_velocity = new_dir;
		angular_velocity = new_da;
	} else {
		double delta_ux = X_controller.calc_control(new_dir.x);
		double delta_uy = Y_controller.calc_control(new_dir.y);
		double delta_ut = T_controller.calc_control(new_da);
	
		linear_velocity = new_dir;
	
		linear_velocity.x = old_control.x + delta_ux;
		linear_velocity.y = old_control.y + delta_uy;
		angular_velocity = old_ang + delta_ut;
	
	}
	old_control = linear_velocity;
	old_ang = angular_velocity;
		
	X_controller.push_history(linear_velocity.x,current_velocity.rotate(-current_orientation).x);
	Y_controller.push_history(linear_velocity.y,current_velocity.rotate(-current_orientation).y);
	T_controller.push_history(angular_velocity,current_angularvel);
}

/**
This is unnessecary because there is not state to clear
in this controller, but it must be implemented.
*/
void JonsController::clear() {
}


RobotControllerFactory &JonsController::get_factory() const {
	return factory;
}

