#include "jons_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "util/timestep.h"
#include <cmath>
#include <iostream>

namespace {

	class jons_controller_factory : public robot_controller_factory {
		public:
			jons_controller_factory() : robot_controller_factory("JONS RC") {
			}

			robot_controller::ptr create_controller(player::ptr plr, bool, unsigned int) const {
				robot_controller::ptr p(new jons_controller(plr));
				return p;
			}
	};

	jons_controller_factory factory;

}



jons_controller::jons_controller(player::ptr plr) : plr(plr), max_acc(10), max_vel(1000), max_Aacc(1), close_param(1.5),position_delta(0.05), orient_delta(0.05)
{
	learning_time=0;
}

void jons_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	const point &current_velocity = plr->est_velocity();
	const point diff = new_position - current_position;
	point new_linear_velocity;	
	double current_angularvel = plr->est_avelocity();
	
	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	
	angular_velocity = new_da;		
	
	point new_dir = diff.rotate(-current_orientation);
	
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
void jons_controller::clear() {
}


robot_controller_factory &jons_controller::get_factory() const {
	return factory;
}

