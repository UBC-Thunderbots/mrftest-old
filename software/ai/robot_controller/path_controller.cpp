#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <cmath>
#include <glibmm.h>
#include <map>

using AI::RC::RobotController;
using AI::RC::TunableController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	const int P = 5;
	// a lower value means higher accuracy, higher is less accurate but faster
	// valid values are 0 to infinity
	const double ACCURACY_TRADEOFF = 0.5;

	const double arr_min[P] = { 3.0, 0.0, 0.0, 3.0, 3.0 };
	const double arr_max[P] = { 6.0, 2.0, 2.0, 6.0, 6.0 };
	/*
	   // robot parameters
	   const double arr_def[P] = { 6.0, .855, .385, 6.0, 6.0 };
	 */
	// simulator parameters
	const double arr_def[P] = { 8.71043, 1.95671, 1.08009, 4.59125, 9.40896 };

	const std::vector<double> param_min(arr_min, arr_min + P);
	const std::vector<double> param_max(arr_max, arr_max + P);
	const std::vector<double> param_default(arr_def, arr_def + P);

	class PathController : public RobotController, public TunableController {
		public:
			PathController(World &world, Player::Ptr player) : RobotController(world, player), param(param_default) {
			}

			void tick() {
				const Player::Path &path = player->path();
				if (path.empty()) {
					return;
				}

				Point new_position = path[path.size() - 1].first.first;
				double new_orientation = path[path.size() - 1].first.second;

				for (Player::Path::const_reverse_iterator i = path.rbegin(), iend = path.rend(); i != iend; ++i) {
					if ((path[0].first.first - player->position()).len() > ACCURACY_TRADEOFF) {
						new_position = i->first.first;
						new_orientation = i->first.second;
					}
				}

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

	class PathControllerFactory : public RobotControllerFactory {
		public:
			PathControllerFactory() : RobotControllerFactory("Path Follower") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr player) const {
				RobotController::Ptr p(new PathController(world, player));
				return p;
			}
	};

	PathControllerFactory factory;
}

