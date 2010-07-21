#include "fuzzy_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <cmath>

namespace {

	class FuzzyControllerFactory : public RobotControllerFactory {
		public:
			FuzzyControllerFactory() : RobotControllerFactory("Fuzzy RC") {
			}

			RobotController::Ptr create_controller(Player::Ptr player, bool, unsigned int) const {
				RobotController::Ptr p(new FuzzyController (player));
				return p;
			}
	};

	FuzzyControllerFactory factory;

	const int P = 5;
	const double arr_min[P] = {3.0, 0.0, 0.0, 3.0, 3.0};
	const double arr_max[P] = {10.0, 2.0, 2.0, 10.0, 10.0};
	//const double arr_def[P] = {9, .855, .385, 9.8, 7.9};
	const double arr_def[P] = {6, .855, .385, 6, 6};
}

const std::vector<double> FuzzyController::param_min(arr_min, arr_min + P);
const std::vector<double> FuzzyController::param_max(arr_max, arr_max + P);
const std::vector<double> FuzzyController::param_default(arr_def, arr_def + P);

const std::vector<double> FuzzyController::get_params_default() const {
	return param_default;
}

FuzzyController::FuzzyController(Player::Ptr player) : param(5) {
	robot = player;
	param = param_default;
}

void FuzzyController::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
	const Point &current_position = robot->position();
	const double current_orientation = robot->orientation();
	angular_velocity = param[4]*angle_mod(new_orientation - current_orientation);
	
	double distance_factor = (new_position - current_position).len() / param[1];
	if (distance_factor > 1) distance_factor = 1;
	
	linear_velocity = (new_position - current_position).rotate(-current_orientation);
	
	if (linear_velocity.len()!=0) linear_velocity = linear_velocity / linear_velocity.len() * distance_factor * param[0];
	
	Point stopping_velocity = (-robot->est_velocity()).rotate(-current_orientation);
	if (stopping_velocity.len()!=0) stopping_velocity = stopping_velocity / stopping_velocity.len() * param[0];
	
	double velocity_factor = ((robot->est_velocity()).len() / param[0]) * param[2];
	if (velocity_factor > 1) velocity_factor = 1;
	
	distance_factor = (new_position - current_position).len() / param[3];
	if (distance_factor > 1) distance_factor = 1;
	
	linear_velocity = distance_factor*linear_velocity+(1-distance_factor)*(velocity_factor*stopping_velocity + (1-velocity_factor)*linear_velocity);
}

void FuzzyController::clear() {
#warning WRITE CODE HERE
}

RobotControllerFactory &FuzzyController::get_factory() const {
	return factory;
}

