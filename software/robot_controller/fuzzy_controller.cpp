#include "fuzzy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class fuzzy_controller_factory : public robot_controller_factory {
		public:
			fuzzy_controller_factory() : robot_controller_factory("Fuzzy RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr player, bool, unsigned int) {
				robot_controller::ptr p(new fuzzy_controller (player));
				return p;
			}
	};

	fuzzy_controller_factory factory;

}

const double BOT_MAX_VELOCITY = 5.0; // <-- this is a parameter which can be tuned

fuzzy_controller::fuzzy_controller(player_impl::ptr player) {
	robot = player;
}

void fuzzy_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = robot->position();
	const double current_orientation = robot->orientation();
	angular_velocity = angle_mod(new_orientation - current_orientation);
	
	double distance_factor = (new_position - current_position).len() / 0.1; // <-- this is a parameter which can be tuned
	if (distance_factor > 1) distance_factor = 1;
	
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
	
	if (linear_velocity.len()!=0) linear_velocity = linear_velocity / linear_velocity.len() * distance_factor * BOT_MAX_VELOCITY;
	
	point stopping_velocity = (-robot->est_velocity()).rotate(-current_orientation);
	if (stopping_velocity.len()!=0) stopping_velocity = stopping_velocity / stopping_velocity.len() * BOT_MAX_VELOCITY;
	
	double velocity_factor = ((robot->est_velocity()).len() / BOT_MAX_VELOCITY) * 4.87; // <-- this is a parameter which can be tuned
	if (velocity_factor > 1) velocity_factor = 1;
	
	distance_factor = (new_position - current_position).len() / 1.02; // <-- this is a parameter which can be tuned
	if (distance_factor > 1) distance_factor = 1;
	
	linear_velocity = distance_factor*linear_velocity+(1-distance_factor)*(velocity_factor*stopping_velocity + (1-velocity_factor)*linear_velocity);
}

robot_controller_factory &fuzzy_controller::get_factory() const {
	return factory;
}

