#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include <cmath>
#include <glibmm.h>
#include <map>

using AI::RC::RobotController;
using AI::RC::TunableController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	const int P = 5;

	const double arr_min[P] = { 3.0, 0.0, 0.0, 3.0, 3.0 };
	const double arr_max[P] = { 8.0, 2.0, 2.0, 8.0, 8.0 };

	// robot parameters
	const double arr_def[P] = {7.72052, 0.261496, 0.385, 7.8411, 7.28714};

	// simulator parameters
	// const double arr_def[P] = { 8.71043, 1.95671, 1.08009, 4.59125, 9.40896 };


	const std::vector<double> param_min(arr_min, arr_min + P);
	const std::vector<double> param_max(arr_max, arr_max + P);
	const std::vector<double> param_default(arr_def, arr_def + P);

	class Fuzzy2Controller : public RobotController, public TunableController {
		public:
			Fuzzy2Controller(World &world, Player::Ptr player) : RobotController(world, player), param(param_default) {
			}

			void tick() {
				const std::vector<std::pair<std::pair<Point, double>, timespec> > &path = player->path();
				if (path.empty()) {
					return;
				}

				Point new_position = path[0].first.first;
				double new_orientation = path[0].first.second;

				const Point &current_position = player->position();
				const double current_orientation = player->orientation();
				double angular_velocity = param[4] * angle_mod(new_orientation - current_orientation);

				double distance_factor = (new_position - current_position).len() / param[1];
				if (distance_factor > 1) {
					distance_factor = 1;
				}

				Point linear_velocity = (new_position - current_position).rotate(-current_orientation);

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

				
				struct timespec currentTime, finalTime;
				currentTime = world.monotonic_time();
				timespec_sub(path[0].second, currentTime, finalTime);
				double desired_velocity = (path[0].first.first - player->position()).len() / finalTime.tv_sec;
				if (linear_velocity.len() > desired_velocity && desired_velocity > 0) {
					LOG_INFO("Warning: Fuzzy controller is being told to travel slow.");
					linear_velocity = desired_velocity * (linear_velocity/linear_velocity.len());
				}

				int wheel_speeds[4] = { 0, 0, 0, 0 };

				convert_to_wheels(linear_velocity, angular_velocity, wheel_speeds);

				player->drive(wheel_speeds);
			}

			void set_params(const std::vector<double> &params) {
				this->param = params;
			}

			const std::vector<double> get_params() const {
				return param;
			}

			const std::vector<double> get_params_default() const {
				return param_default;
			}

			const std::vector<double> get_params_min() const {
				return param_min;
			}

			const std::vector<double> get_params_max() const {
				return param_max;
			}

		protected:
			std::vector<double> param;
	};

	class Fuzzy2ControllerFactory : public RobotControllerFactory {
		public:
			Fuzzy2ControllerFactory() : RobotControllerFactory("Fuzzy Version 2") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr player) const {
				RobotController::Ptr p(new Fuzzy2Controller(world, player));
				return p;
			}
	};

	Fuzzy2ControllerFactory factory;
}

