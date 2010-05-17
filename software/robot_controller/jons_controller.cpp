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
}

void jons_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	const point &current_velocity = plr->est_velocity();
	const point diff = new_position - current_position;
	point new_linear_velocity;	
	double current_angularvel = plr->est_avelocity();
	double vel_in_dir_travel;
	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	
	//if(plr->est_acceleration().len() > max_acc)
	//	max_acc=max_acc + plr->est_acceleration().len();

	//if(abs(plr->est_aacceleration()) > max_Aacc)
	//	max_Aacc = abs(plr->est_aacceleration());

	if(pow(current_angularvel,2)/max_Aacc*close_param < fabs(new_da) && fabs(new_da) > orient_delta)
		angular_velocity = new_da/fabs(new_da)*max_vel;
	else
		angular_velocity=0;
		
	
	point new_dir = diff.rotate(-current_orientation);
	
	//if (new_da > PI) new_da -= 2 * PI;
	if(diff.len() > 0.0001) {
	
		new_dir = new_dir / new_dir.len();
		vel_in_dir_travel=current_velocity.dot(diff/diff.len());
	}else {
		vel_in_dir_travel=0;
	}
	
	if(pow(vel_in_dir_travel,2)/max_acc*close_param < diff.len() && diff.len() > position_delta)
		linear_velocity = max_vel*new_dir;
	else
		linear_velocity = new_dir*0;
		
	//Fix the problems with unrealistic numbers when the robot is manually moved
	//if(isnan(linear_velocity.len()) || isinf(linear_velocity.len()) || isinf(-linear_velocity.len()))
	//	linear_velocity = point(0,0);
			
	//if(isnan(angular_velocity) || isinf(angular_velocity) || isinf(-angular_velocity))
	//	angular_velocity=0;
}

/**
This is unnessecary because there is not state to clear
in this controller.
*/
void jons_controller::clear() {
}

robot_controller_factory &jons_controller::get_factory() const {
	return factory;
}

