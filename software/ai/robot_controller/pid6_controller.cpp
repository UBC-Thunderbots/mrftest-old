#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include "util/timestep.h"
#include <cmath>
#include <glibmm.h>
#include <iostream>
#include <vector>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	DoubleParam wheel_max_speed("Limit wheel speed (quarter degree per 5 ms)", "RC/PID6", 330.0, 0, 1023);
	DoubleParam wheel_max_accel("Limit wheel accel (quarter degree per 5 ms squared)", "RC/PID6", 45, 0, 1023);
	DoubleParam motor_to_field("Ratio of motor distance to field distance", "RC/PID6", 0.00633, 0.001, 1);

	class PID6Controller : public RobotController {
		public:
			void tick();
			void move(const Point &new_position, double new_orientation, int(&wheel_speeds)[4]);
			void clear();
			RobotControllerFactory &get_factory() const;
			PID6Controller(World &world, Player::Ptr plr);

		protected:
			double prev_speed[4];
	};

	PID6Controller::PID6Controller(World &world, Player::Ptr plr) : RobotController(world, plr) {
		for (unsigned i = 0; i < 4; ++i) {
			prev_speed[i] = 0;
		}
	}

	void PID6Controller::tick() {
		const AI::RC::W::Player::Path &path = player->path();
		if (path.empty()) {
			clear();
		} else {
			int wheels[4];
			move(path[0].first.first, path[0].first.second, wheels);
			player->drive(wheels);
		}
	}

	void PID6Controller::move(const Point &new_position, double new_orientation, int(&wheel_speeds)[4]) {
		static const double WHEEL_MATRIX[4][3] = {
			{ -42.5995, 27.6645, 4.3175 },
			{ -35.9169, -35.9169, 4.3175 },
			{ 35.9169, -35.9169, 4.3175 },
			{ 42.5995, 27.6645, 4.3175 }
		};

		double max_acc = 200.0 / TIMESTEPS_PER_SECOND * wheel_max_accel;
		double distance_to_velocity = 2 * max_acc / wheel_max_speed / motor_to_field;

		Point position_error = (new_position - player->position()).rotate(-player->orientation());
		double angular_error = angle_mod(new_orientation - player->orientation());

		const double position_delta[3] = { position_error.x, position_error.y, angular_error };
		double wheel_target_vel[4] = { 0, 0, 0, 0 };
		double vel_error[4] = { 0, 0, 0, 0 };

		for (unsigned int row = 0; row < 4; ++row) {
			for (unsigned int col = 0; col < 3; ++col) {
				wheel_target_vel[row] += WHEEL_MATRIX[row][col] * position_delta[col];
			}
			wheel_target_vel[row] = distance_to_velocity * wheel_target_vel[row];
			vel_error[row] = wheel_target_vel[row] - prev_speed[row];
		}

		double max_diff = 0;
		for (unsigned int i = 0; i < 4; i++) {
			max_diff = std::max(max_diff, std::fabs(vel_error[i]));
		}

		double ratio = wheel_max_accel / max_diff;
		for (unsigned int i = 0; i < 4; i++) {
			if (max_diff > wheel_max_accel) {
				vel_error[i] = ratio * vel_error[i];
			}
			wheel_target_vel[i] += vel_error[i];
		}

		max_diff = 0;
		for (unsigned int i = 0; i < 4; i++) {
			max_diff = std::max(max_diff, std::fabs(wheel_target_vel[i]));
		}

		ratio = wheel_max_speed / max_diff;
		for (unsigned int i = 0; i < 4; i++) {
			if (max_diff > wheel_max_speed) {
				wheel_speeds[i] = ratio * wheel_target_vel[i];
			} else {
				wheel_speeds[i] = wheel_target_vel[i];
			}
		}
	}


	void PID6Controller::clear() {
	}

	class PID6ControllerFactory : public RobotControllerFactory {
		public:
			PID6ControllerFactory() : RobotControllerFactory("PID 6") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr plr) const {
				RobotController::Ptr p(new PID6Controller(world, plr));
				return p;
			}
	};

	PID6ControllerFactory factory;

	RobotControllerFactory &PID6Controller::get_factory() const {
		return factory;
	}
}

