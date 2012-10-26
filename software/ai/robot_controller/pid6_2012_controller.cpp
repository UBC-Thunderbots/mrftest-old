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
#include <iostream>
#include <cmath>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
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
					max = std::max(max, std::abs(direction[i]));
				}

				if (max < limit_val) {
					return *this;
				}

				double ratio = limit_val / max;

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

			Vector4 map(std::function<double(const double &)> x) {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = x(direction[i]);
				}
				return temp;
			}
			
			void print() {
				for(int i=0;i<4;i++) {
					std::cout << direction[i];
					if(i != 3)
						std::cout << ":";
				}
				std::cout << std::endl;
			}
	};

	class Vector3 {
		public:
			Point cartesian_direction;
			Angle angular_direction;
			static const double WHEEL_MATRIX[4][3];

			explicit Vector3() : cartesian_direction(0, 0), angular_direction(Angle::ZERO) {
			}

			explicit Vector3(const Point &cart, const Angle &angle) : cartesian_direction(cart), angular_direction(angle) {}

			Vector3 operator*(double scale_value) const {
				return Vector3(cartesian_direction * scale_value, (angular_direction * scale_value).angle_mod());
			}

			Vector3 operator/(double scale_value) const {
				return Vector3(cartesian_direction / scale_value, angular_direction / scale_value);
			}

			Vector3 operator+(const Vector3 &sum) const {
				return Vector3(cartesian_direction + sum.cartesian_direction, (angular_direction + sum.angular_direction).angle_mod());
			}

			Vector3 operator-(const Vector3 &sub) const {
				return Vector3(cartesian_direction - sub.cartesian_direction, (angular_direction - sub.angular_direction).angle_mod());
			}

			Vector3 rebase(const Vector3& new_origin) const {
				Point p_diff = cartesian_direction - new_origin.cartesian_direction;
				Angle a_diff = (angular_direction - new_origin.angular_direction).angle_mod();
				return Vector3(p_diff.rotate(-new_origin.angular_direction),a_diff);
			}

			Vector4 toVector4() const {
				Vector4 temp;
				for (int i = 0; i < 4; i++) {
					temp.direction[i] = WHEEL_MATRIX[i][0] * cartesian_direction.x + WHEEL_MATRIX[i][1] * cartesian_direction.y + WHEEL_MATRIX[i][2] * angular_direction.to_radians();
				}
				return temp;
			}
	};

	DoubleParam firmware_loop_rate("Tick rate of firmware control loop in s^-1", "RC/PID6_2012", 200.0, 0.0, 48.0e6);
	DoubleParam wheel_max_speed("Limit wheel speed (quarter degree per firmware tick)", "RC/PID6_2012", 257.0, 0, 1023);
	DoubleParam wheel_max_accel("Limit wheel accel (quarter degree per firmware tick squared)", "RC/PID6_2012", 75, 0, 1023);
	DoubleParam aggressiveness("Aggressiveness of the controller", "RC/PID6_2012", 0.8, 0, 1.0);

	class PID6_2012Controller : public RobotController {
		public:
			void tick();
			void move(const Point &new_position, Angle new_orientation, timespec time_of_arrival, int(&wheel_speeds)[4]);
			void clear();
			explicit PID6_2012Controller(World world, Player plr);

		protected:
			Vector4 prev_speed;
	};

	PID6_2012Controller::PID6_2012Controller(World world, Player plr) : RobotController(world, plr) {
		for (unsigned i = 0; i < 4; ++i) {
			prev_speed.direction[i] = 0;
		}
	}

	void PID6_2012Controller::tick() {
		const AI::RC::W::Player::Path &path = player->path();
		if (path.empty()) {
			clear();
		} else {
			int wheels[4];
			move(path[0].first.first, path[0].first.second, path[0].second, wheels);
			player->drive(wheels);
		}
	}

	double distance_to_velocity(const double &distance) {
			double max_distance = wheel_max_speed * wheel_max_speed / 2.0 / wheel_max_accel / aggressiveness;

			return distance * firmware_loop_rate / max_distance * wheel_max_speed;

//		return distance * dist_to_vel; // the velocity
			
//			return std::copysign(std::sqrt(firmware_loop_rate*std::abs(distance)*2/wheel_max_accel*aggressiveness),distance);
			
//			return distance * aggressiveness;
	}

	void PID6_2012Controller::move(const Point &new_point, Angle new_orientation, timespec time_of_arrival, int(&wheel_speeds)[4]) {
		// This is the difference between where we are and where we are going rotated to robot coordinates
		Vector3 current_position = Vector3(player->position(),player->orientation());
		Vector3 new_position = Vector3(new_point,new_orientation);
		Vector3 position_delta = new_position.rebase(current_position);

		double time_deadline = timespec_to_double(timespec_sub(time_of_arrival, world.monotonic_time()));

		time_deadline = (time_deadline < 1.0 / TIMESTEPS_PER_SECOND) ? 1.0 / TIMESTEPS_PER_SECOND : time_deadline;

		//toVector4 from a meter delta provides s*q/t therefore divide by seconds
		Vector4 wheel_target_vel = position_delta.toVector4().map(distance_to_velocity) / time_deadline;
	
		//vel error in q/t
		Vector4 vel_error = wheel_target_vel - prev_speed;
		wheel_target_vel = prev_speed + vel_error.limit(wheel_max_accel * firmware_loop_rate / TIMESTEPS_PER_SECOND);

		wheel_target_vel = wheel_target_vel.limit(wheel_max_speed);

		wheel_target_vel.get(wheel_speeds);
		prev_speed = wheel_target_vel;
	}

	void PID6_2012Controller::clear() {
	}
}

ROBOT_CONTROLLER_REGISTER(PID6_2012Controller)

const double Vector3::WHEEL_MATRIX[4][3] = {
			{	-71.85,	46.66,	7.06},
			{	-60.58,	-60.58,	7.06},
			{	60.58,	-60.58,	7.06},
			{	71.85,	46.68,	7.06}
};
