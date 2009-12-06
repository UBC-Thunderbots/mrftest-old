#include "jons_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class jons_controller_factory : public robot_controller_factory {
		public:
			jons_controller_factory() : robot_controller_factory("JONS RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr plr, bool, unsigned int) {
				robot_controller::ptr p(new jons_controller(plr));
				return p;
			}
	};

	jons_controller_factory factory;

}

jons_controller::jons_controller(player_impl::ptr plr) : plr(plr), max_acc(10), max_vel(8000), max_Aacc(1)
{
}

void jons_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	const point &current_velocity = plr->est_velocity();
	const point diff = new_position - current_position;
	double vel_in_dir_travel;
	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	point new_dir = diff.rotate(-current_orientation);
	new_dir /= new_dir.len();
	if (new_da > PI) new_da -= 2 * PI;
	angular_velocity = new_da;
	vel_in_dir_travel=current_velocity.dot(diff/diff.len());
	if(pow(vel_in_dir_travel,2)/max_acc*1.5 > (new_position-current_position).len())
		linear_velocity = max_vel*new_dir;
	else
		linear_velocity = 0*new_dir;
	
}

robot_controller_factory &jons_controller::get_factory() const {
	return factory;
}

