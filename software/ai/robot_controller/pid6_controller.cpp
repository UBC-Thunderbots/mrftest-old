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
#include <functional>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	class Vector4 {
		public:
			double direction[4];

			void get(int* vector) {
				for(int i=0;i<4;i++) {
					vector[i] = static_cast<int>(direction[i]);
				}		
			}
		
			Vector4 limit(const double &limit_val) {
				double max = -1.0/0.0;
				for(int i=0;i<4;i++) {
					max = std::max(max,direction[i]);
				}
				
				if(std::abs(max) <  limit_val) {
					return *this;
				}
			
				double ratio = limit_val / std::abs(max);

				Vector4 temp;
				for(int i=0;i<4;i++) {
					temp.direction[i] = direction[i]*ratio;
				}
				return temp;
			}

			Vector4 operator*(const double& scale) const {
					Vector4 temp;
					for(int i=0;i<4;i++) {
						temp.direction[i] = direction[i]*scale;
					}	
					return temp;
			}
			
			Vector4 operator/(const double& scale) const {
				return (*this)*(1.0/scale);
			}
		
			Vector4 operator+(const Vector4& other) const {
				Vector4 temp;
				for(int i=0;i<4;i++) {
					temp.direction[i] = direction[i] + other.direction[i];
				}
				return temp;
			}
			
			Vector4 operator-(const Vector4& other) const {
				Vector4 temp;
				for(int i=0;i<4;i++) {
					temp.direction[i] = direction[i] - other.direction[i];
				}
				return temp;
			}

			Vector4 map(std::function<double(double)> x) {
				Vector4 temp;
				for(int i=0;i<4;i++) {
					temp.direction[i] = x(temp.direction[i]);
				}
				return temp;
			}
			
			friend Vector4 operator*(const double& , const Vector4&);
	};
	
	Vector4 operator*(const double& scale, const Vector4& vec) {
		return vec*scale;
	}
	
	class Vector3 {
		public:
			Point cartesian_direction;
			double angular_direction;
			static const double WHEEL_MATRIX[4][3];

			Vector3() : cartesian_direction(Point(0,0)),angular_direction(0.0) {}

			Vector3(const Point& cart, const double& angle) : cartesian_direction(cart), angular_direction(angle) {}

			Vector3 operator*(double scale_value) const {
				return Vector3(cartesian_direction*scale_value,angular_direction*scale_value);
			}
			Vector3 operator/(double scale_value) const {
				return Vector3(cartesian_direction/scale_value,angular_direction/scale_value);
			}

			Vector3 operator+(const Vector3& sum) const {
				return Vector3(cartesian_direction + sum.cartesian_direction,angular_direction + sum.angular_direction);
			}
	
			Vector3 operator-(const Vector3& sub) const {
				return Vector3(cartesian_direction - sub.cartesian_direction,angular_direction - sub.angular_direction);
			}

			Vector4 toVector4() const {
				Vector4 temp;
				for(int i=0;i<4;i++) {
					temp.direction[i] = WHEEL_MATRIX[i][0] * cartesian_direction.x + WHEEL_MATRIX[i][1] * cartesian_direction.y + WHEEL_MATRIX[i][2] * angular_direction;
				}
				return temp;
			}

			friend Vector3 operator*(const double&, const Vector3&);
	};

	
	
	Vector3 operator*(const double& scale, const Vector3& vec) {
		return vec*scale;
	}

	DoubleParam firmware_loop_rate("Tick rate of firmware control loop in s^-1","RC/PID6",200.0,0.0,48.0e6);
	DoubleParam wheel_max_speed("Limit wheel speed (quarter degree per firmware tick)", "RC/PID6", 330.0, 0, 1023);
	DoubleParam wheel_max_accel("Limit wheel accel (quarter degree per firmware tick squared)", "RC/PID6", 45, 0, 1023);
	DoubleParam aggressiveness("Aggressiveness of the controller","RC/PID6",0.8,0,1.0);

	class PID6Controller : public RobotController {
		public:
			void tick();
			void move(const Point &new_position, Angle new_orientation, timespec time_of_arrival, int(&wheel_speeds)[4]);
			void clear();
			RobotControllerFactory &get_factory() const;
			PID6Controller(World &world, Player::Ptr plr);

		protected:
			Vector4 prev_speed;
	};

	PID6Controller::PID6Controller(World &world, Player::Ptr plr) : RobotController(world, plr) {
		for (unsigned i = 0; i < 4; ++i) {
			prev_speed.direction[i] = 0;
		}
	}

	void PID6Controller::tick() {
		const AI::RC::W::Player::Path &path = player->path();
		if (path.empty()) {
			clear();
		} else {
			int wheels[4];
			move(path[0].first.first, path[0].first.second, path[0].second,wheels);
			player->drive(wheels);
		}
	}

	static double distance_to_velocity(const double& distance) {
		double max_acc = firmware_loop_rate / TIMESTEPS_PER_SECOND * wheel_max_accel;
		double dist_to_vel = 2 * max_acc / wheel_max_speed * aggressiveness;

		return distance*dist_to_vel; //the velocity
	}

	void PID6Controller::move(const Point &new_position, Angle new_orientation, timespec time_of_arrival, int(&wheel_speeds)[4]) {
		//This is the difference between where we are and where we are going rotated to robot coordinates
		Vector3 position_delta = Vector3((new_position - player->position()).rotate(-player->orientation()),(new_orientation - player->orientation()).angle_mod().to_radians());

		double time_deadline = timespec_to_double(timespec_sub(time_of_arrival,world.monotonic_time()));
		
		time_deadline = (time_deadline < 1.0/TIMESTEPS_PER_SECOND)?1.0/TIMESTEPS_PER_SECOND:time_deadline; 
		
		Vector4 wheel_target_vel = position_delta.toVector4().map(distance_to_velocity)/time_deadline;
		Vector4 vel_error = wheel_target_vel - prev_speed;
	
		wheel_target_vel = prev_speed + vel_error.limit(wheel_max_accel*firmware_loop_rate/TIMESTEPS_PER_SECOND);

		wheel_target_vel = wheel_target_vel.limit(wheel_max_speed);

		wheel_target_vel.get(wheel_speeds);
		prev_speed = wheel_target_vel;
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

const double Vector3::WHEEL_MATRIX[4][3] = {
	{ -42.5995, 27.6645, 4.3175 },
	{ -35.9169, -35.9169, 4.3175 },
	{ 35.9169, -35.9169, 4.3175 },
	{ 42.5995, 27.6645, 4.3175 }
};
