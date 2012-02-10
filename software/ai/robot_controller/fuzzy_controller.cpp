#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	class FuzzyController : public OldRobotController, public AI::RC::TunableController {
		public:
			void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity);

			void clear();

			explicit FuzzyController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player);

			void set_params(const std::vector<double> &params) {
				this->param = params;
			}

			const std::vector<double> get_params() const {
				return param;
			}

			const std::vector<double> get_params_default() const;

			const std::vector<double> get_params_min() const {
				return param_min;
			}

			const std::vector<double> get_params_max() const {
				return param_max;
			}

		protected:
			static const std::vector<double> param_min;
			static const std::vector<double> param_max;
			static const std::vector<double> param_default;

			std::vector<double> param;
	};

	const int P = 5;
	const double arr_min[P] = { 3.0, 0.0, 0.0, 3.0, 3.0 };
	const double arr_max[P] = { 10.0, 2.0, 2.0, 10.0, 10.0 };
	// const double arr_def[P] = {9, .855, .385, 9.8, 7.9};
	const double arr_def[P] = { 6, .855, .385, 6, 6 };
}

const std::vector<double> FuzzyController::param_min(arr_min, arr_min + P);
const std::vector<double> FuzzyController::param_max(arr_max, arr_max + P);
const std::vector<double> FuzzyController::param_default(arr_def, arr_def + P);

const std::vector<double> FuzzyController::get_params_default() const {
	return param_default;
}

FuzzyController::FuzzyController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player) : OldRobotController(world, player), param(5) {
	param = param_default;
}

void FuzzyController::move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) {
	const Point &current_position = player->position();
	const Angle current_orientation = player->orientation();
	angular_velocity = param[4] * (new_orientation - current_orientation).angle_mod();

	double distance_factor = (new_position - current_position).len() / param[1];
	if (distance_factor > 1) {
		distance_factor = 1;
	}

	linear_velocity = (new_position - current_position).rotate(-current_orientation);

	if (linear_velocity.len() != 0) {
		linear_velocity = linear_velocity / linear_velocity.len() * distance_factor * param[0];
	}

	Point stopping_velocity = (-player->velocity()).rotate(-current_orientation);
	if (stopping_velocity.len() != 0) {
		stopping_velocity = stopping_velocity / stopping_velocity.len() * param[0];
	}

	double velocity_factor = ((player->velocity()).len() / param[0]) * param[2];
	if (velocity_factor > 1) {
		velocity_factor = 1;
	}

	distance_factor = (new_position - current_position).len() / param[3];
	if (distance_factor > 1) {
		distance_factor = 1;
	}

	linear_velocity = distance_factor * linear_velocity + (1 - distance_factor) * (velocity_factor * stopping_velocity + (1 - velocity_factor) * linear_velocity);
}

void FuzzyController::clear() {
}

ROBOT_CONTROLLER_REGISTER(FuzzyController)

