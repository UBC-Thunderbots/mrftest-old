#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include "util/timestep.h"
#include <cmath>
#include <functional>
#include <vector>

using AI::RC::RobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	class Vector4 {
		public:
			double direction[4];

			void get(int *vector) const {
				for (int i = 0; i < 4; i++) {
					vector[i] = static_cast<int>(direction[i]);
				}
			}

			Vector4 limit(const double &limit_val) {
				double max = -1.0 / 0.0;
				for (int i = 0; i < 4; i++) {
					max = std::max(max, direction[i]);
				}

				if (std::abs(max) < limit_val) {
					return *this;
				}

				double ratio = limit_val / std::abs(max);

				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = direction[i] * ratio;
				}
				return temp;
			}

			Vector4 operator*(const double &scale) const {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = direction[i] * scale;
				}
				return temp;
			}

			Vector4 operator/(const double &scale) const {
				return (*this) * (1.0 / scale);
			}

			Vector4 operator+(const Vector4 &other) const {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = direction[i] + other.direction[i];
				}
				return temp;
			}

			Vector4 operator-(const Vector4 &other) const {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = direction[i] - other.direction[i];
				}
				return temp;
			}

			Vector4 map(std::function<double(double)> x) {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = x(temp.direction[i]);
				}
				return temp;
			}
	};

	class Vector3 {
		public:
			Point cartesian_direction;
			double angular_direction;

			explicit Vector3() : cartesian_direction(0, 0), angular_direction(0.0) {
			}

			explicit Vector3(const Point &cart, const double &angle) : cartesian_direction(cart), angular_direction(angle) {}

			Vector3 operator*(double scale_value) const {
				return Vector3(cartesian_direction * scale_value, angular_direction * scale_value);
			}

			Vector3 operator/(double scale_value) const {
				return Vector3(cartesian_direction / scale_value, angular_direction / scale_value);
			}

			Vector3 operator+(const Vector3 &sum) const {
				return Vector3(cartesian_direction + sum.cartesian_direction, angular_direction + sum.angular_direction);
			}

			Vector3 operator-(const Vector3 &sub) const {
				return Vector3(cartesian_direction - sub.cartesian_direction, angular_direction - sub.angular_direction);
			}

			Vector4 toVector4() const {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = AI::RC::RobotController::WHEEL_MATRIX[i][0] * cartesian_direction.x + AI::RC::RobotController::WHEEL_MATRIX[i][1] * cartesian_direction.y + AI::RC::RobotController::WHEEL_MATRIX[i][2] * angular_direction;
				}
				return temp;
			}
	};

	DoubleParam firmware_loop_rate(u8"Tick rate of firmware control loop in s^-1", u8"RC/PID6", 200.0, 0.0, 48.0e6);
	DoubleParam wheel_max_speed(u8"Limit wheel speed (quarter degree per firmware tick)", u8"RC/PID6", 330.0, 0, 1023);
	DoubleParam wheel_max_accel(u8"Limit wheel accel (quarter degree per firmware tick squared)", u8"RC/PID6", 45, 0, 1023);
	DoubleParam aggressiveness(u8"Aggressiveness of the controller", u8"RC/PID6", 0.8, 0, 1.0);

	class PID6Controller : public RobotController {
		public:
			void tick();
			void move(const Point &new_position, Angle new_orientation, AI::Timestamp time_of_arrival, int(&wheel_speeds)[4]);
			void clear();
			explicit PID6Controller(World world, Player plr);

		protected:
			Vector4 prev_speed;
	};

	PID6Controller::PID6Controller(World world, Player plr) : RobotController(world, plr) {
		for (unsigned i = 0; i < 4; ++i) {
			prev_speed.direction[i] = 0;
		}
	}

	void PID6Controller::tick() {
		const AI::RC::W::Player::Path &path = player.path();
		if (path.empty()) {
			clear();
		} else {
			int wheels[4];
			move(path[0].first.first, path[0].first.second, path[0].second, wheels);
			player.drive(wheels);
		}
	}

	double distance_to_velocity(const double &distance) {
		double max_acc = firmware_loop_rate / TIMESTEPS_PER_SECOND * wheel_max_accel;
		double dist_to_vel = 2 * max_acc / wheel_max_speed * aggressiveness;

		return distance * dist_to_vel; // the velocity
	}

	void PID6Controller::move(const Point &new_position, Angle new_orientation, AI::Timestamp time_of_arrival, int(&wheel_speeds)[4]) {
		// This is the difference between where we are and where we are going rotated to robot coordinates
		Vector3 position_delta = Vector3((new_position - player.position()).rotate(-player.orientation()), (new_orientation - player.orientation()).angle_mod().to_radians());

		double time_deadline = std::chrono::duration_cast<std::chrono::duration<double>>(time_of_arrival - world.monotonic_time()).count();

		time_deadline = (time_deadline < 1.0 / TIMESTEPS_PER_SECOND) ? 1.0 / TIMESTEPS_PER_SECOND : time_deadline;

		Vector4 wheel_target_vel = position_delta.toVector4().map(distance_to_velocity) / time_deadline;
		Vector4 vel_error = wheel_target_vel - prev_speed;

		wheel_target_vel = prev_speed + vel_error.limit(wheel_max_accel * firmware_loop_rate / TIMESTEPS_PER_SECOND);

		wheel_target_vel = wheel_target_vel.limit(wheel_max_speed);

		wheel_target_vel.get(wheel_speeds);
		prev_speed = wheel_target_vel;
	}

	void PID6Controller::clear() {
	}
}

ROBOT_CONTROLLER_REGISTER(PID6Controller)

