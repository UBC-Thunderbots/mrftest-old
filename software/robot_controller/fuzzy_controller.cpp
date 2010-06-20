#include "fuzzy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

namespace {

	class fuzzy_controller_factory : public robot_controller_factory {
		public:
			fuzzy_controller_factory() : robot_controller_factory("Fuzzy RC") {
			}

			robot_controller::ptr create_controller(player::ptr player, bool, unsigned int) const {
				robot_controller::ptr p(new fuzzy_controller (player));
				return p;
			}
	};

	fuzzy_controller_factory factory;

	const int P = 5;
	const double arr_min[P] = {0.0, 0.0, 0.0, 0.0, 0.0};
	const double arr_max[P] = {10.0, 10.0, 10.0, 10.0, 10.0};
	const double arr_def[P] = {25.4088, 0.855547, 4.91968, 2.13349, 2.0};
}

const std::vector<double> fuzzy_controller::param_min(arr_min, arr_min + P);
const std::vector<double> fuzzy_controller::param_max(arr_max, arr_max + P);
const std::vector<double> fuzzy_controller::param_default(arr_def, arr_def + P);

fuzzy_controller::fuzzy_controller(player::ptr player) : param(4) {
	robot = player;
	param = param_default;
}

void fuzzy_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = robot->position();
	const double current_orientation = robot->orientation();
	angular_velocity = param[4]*angle_mod(new_orientation - current_orientation);
	
	double distance_factor = (new_position - current_position).len() / param[1];
	if (distance_factor > 1) distance_factor = 1;
	
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
	
	if (linear_velocity.len()!=0) linear_velocity = linear_velocity / linear_velocity.len() * distance_factor * param[0];
	
	point stopping_velocity = (-robot->est_velocity()).rotate(-current_orientation);
	if (stopping_velocity.len()!=0) stopping_velocity = stopping_velocity / stopping_velocity.len() * param[0];
	
	double velocity_factor = ((robot->est_velocity()).len() / param[0]) * param[2];
	if (velocity_factor > 1) velocity_factor = 1;
	
	distance_factor = (new_position - current_position).len() / param[3];
	if (distance_factor > 1) distance_factor = 1;
	
	linear_velocity = distance_factor*linear_velocity+(1-distance_factor)*(velocity_factor*stopping_velocity + (1-velocity_factor)*linear_velocity);
}

void fuzzy_controller::clear() {
#warning WRITE CODE HERE
}

robot_controller_factory &fuzzy_controller::get_factory() const {
	return factory;
}

