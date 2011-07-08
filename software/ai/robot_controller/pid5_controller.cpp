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

	DoubleParam pid_xy_prop("xy +proportional", "RC/PID5", 25.0, 0.0, 1000.0);
	DoubleParam pid_xy_diff("xy -differential", "RC/PID5", 0.0, -100.0, 1000.0);

	DoubleParam pid_a_prop("angular +proportional", "RC/PID5", 30.0, 0.0, 8888);
	DoubleParam pid_a_diff("angular -differential", "RC/PID5", 0.2, -8888, 8888);

	DoubleParam pid_xy_ratio("x to y ratio", "RC/PID5", 0.81, 0.0, 2.0);

	DoubleParam pid_ya_ratio("YA ratio", "RC/PID5", 0.08, -10.0, 10.0);

	DoubleParam wheel_max_speed("Limit wheel speed", "RC/PID5", 100.0, 0, 8888);
	DoubleParam wheel_max_accel("Limit wheel accel", "RC/PID5", 5.0, 0, 8888);

	class PID5Controller : public RobotController {
		public:
			void tick();
			void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);
			void move(const Point &new_position, double new_orientation, int(&wheel_speeds)[4]);
			void clear();
			void convert(const Point &vel, double avel, int(&wheel_speeds)[4]);
			RobotControllerFactory &get_factory() const;
			PID5Controller(World &world, Player::Ptr plr);

		protected:
			double prev_speed[4];
	};

	PID5Controller::PID5Controller(World &world, Player::Ptr plr) : RobotController(world, plr) {

		for (unsigned i = 0; i < 4; ++i) {
			prev_speed[i] = 0;
		}
	}

	void PID5Controller::convert(const Point &vel, double avel, int(&wheel_speeds)[4]) {
		static const double WHEEL_MATRIX[4][3] = {
			{ -42.5995, 27.6645, 4.3175 },
			{ -35.9169, -35.9169, 4.3175 },
			{ 35.9169, -35.9169, 4.3175 },
			{ 42.5995, 27.6645, 4.3175 }
		};
		const double input[3] = { vel.x, vel.y, avel };
		double output[4] = { 0, 0, 0, 0 };
		for (unsigned int row = 0; row < 4; ++row) {
			for (unsigned int col = 0; col < 3; ++col) {
				output[row] += WHEEL_MATRIX[row][col] * input[col];
			}
		}

		double max_speed = 0;
		for (unsigned int row = 0; row < 4; ++row) {
			max_speed = std::max(max_speed, std::fabs(output[row]));
		}

		if (max_speed > wheel_max_speed) {
			double ratio = wheel_max_speed / max_speed;
			for (unsigned int row = 0; row < 4; ++row) {
				output[row] *= ratio;
			}
		}

		double accel[4];

		double max_accel = 0;
		for (unsigned int row = 0; row < 4; ++row) {
			accel[row] = output[row] - prev_speed[row];
			max_accel = std::max(max_accel, std::fabs(accel[row]));
		}

		if (max_accel > wheel_max_accel) {
			for (unsigned int i = 0; i < 4; ++i) {
				accel[i] /= max_accel;
				accel[i] *= wheel_max_accel;
				output[i] = prev_speed[i] + accel[i];
			}
		}

		for (unsigned int row = 0; row < 4; ++row) {
			wheel_speeds[row] = static_cast<int>(output[row]);
		}
	}

	void PID5Controller::tick() {
		const AI::RC::W::Player::Path &path = player->path();
		if (path.empty()) {
			clear();
		} else {
			int wheels[4];
			move(path[0].first.first, path[0].first.second, wheels);
			player->drive(wheels);
		}
	}

	void PID5Controller::move(const Point &new_position, double new_orientation, int(&wheel_speeds)[4]) {
		Point vel;
		double avel;
		move(new_position, new_orientation, vel, avel);
		convert(vel, avel, wheel_speeds);
	}

	void PID5Controller::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
		const Point &current_position = player->position();
		const double current_orientation = player->orientation();

		// relative new direction and angle
		double new_da = angle_mod(new_orientation - current_orientation);
		const Point &new_dir = (new_position - current_position).rotate(-current_orientation);

		if (new_da > M_PI) {
			new_da -= 2 * M_PI;
		}

		const double px = new_dir.x;
		const double py = new_dir.y;
		const double pa = new_da;
		Point vel = (player->velocity()).rotate(-current_orientation);
		double vx = -vel.x;
		double vy = -vel.y;
		double va = -player->avelocity();

		linear_velocity.x = px * pid_xy_prop + vx * pid_xy_diff;
		linear_velocity.y = (py * pid_xy_prop + vy * pid_xy_diff) * pid_xy_ratio;

		angular_velocity = pa * pid_a_prop + va * pid_a_diff + linear_velocity.y * pid_ya_ratio;
	}

	void PID5Controller::clear() {
	}

	class PID5ControllerFactory : public RobotControllerFactory {
		public:
			PID5ControllerFactory() : RobotControllerFactory("PID 5") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr plr) const {
				RobotController::Ptr p(new PID5Controller(world, plr));
				return p;
			}
	};

	PID5ControllerFactory factory;

	RobotControllerFactory &PID5Controller::get_factory() const {
		return factory;
	}
}

