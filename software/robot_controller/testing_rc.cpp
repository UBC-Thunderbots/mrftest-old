#include "ai/world/player.h"
#include "geom/angle.h"
#include "robot_controller/robot_controller.h"

namespace {
	//
	// The robot controller.
	//
	class TestingRC : public RobotController {
		public:
			//
			// Constructs a new controller.
			//
			TestingRC(Player::Ptr plr);

			//
			// Constructs a new controller.
			//
			// Parameters:
			//  target_position
			//   location the player wants to be in World coordinate
			//
			//  target_orientation
			//   direction the player wants to have
			//
			void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

			//
			// Clears accumulated data.
			//
			void clear() {
			}

			//
			// Returns the factory.
			//
			RobotControllerFactory &get_factory() const;

		private:
			Player::Ptr plr;

			double get_velocity(double d, double v0, double v1, double max_vel, double max_accel);

			bool initialized;

			Point old_position;
			double old_orientation;

			double time_step;

			double max_angular_velocity;
			double max_angular_velocity_accel;

			double max_linear_velocity;
			double max_linear_velocity_accel;
	};

	TestingRC::TestingRC(Player::Ptr plr) : plr(plr), initialized(false) {
	}

	double TestingRC::get_velocity(double s, double v0, double v1, double max_vel, double max_accel) {
		// std::cout << s << ' ' << v0 << ' ' << v1 << ' ' << max_vel << ' '<< max_accel << std::endl;
		if (s == 0 && v0 == v1) return 0;
		if (s < 0) return get_velocity(-s, -v0, -v1, max_vel, max_accel);

		if (v0 > max_vel) v0 = max_vel;
		else if (v0 < -max_vel) v0 = -max_vel;
		if (v1 > max_vel) v1 = max_vel;
		else if (v1 < -max_vel) v1 = -max_vel;

		// there are 4 cases:
		// Case 1: accel, cruise, decel
		double t_accel = (max_vel - v0) / max_accel;
		double s_accel = max_accel * t_accel * t_accel / 2 + v0 * t_accel;
		double t_decel = (max_vel - v1) / max_accel;
		double s_decel = -max_accel * t_decel * t_decel / 2 + v1 * t_decel;
		if (s > s_accel + s_decel) {
			// t = (s - s_accel - s_decel) / max_vel + t_accel + t_decel;
			return v0 + max_accel * time_step;
		}

		// Case 2: decel, cruise, accel
		t_decel = (v0 + max_vel) / max_accel;
		s_decel = -max_accel * t_decel * t_decel / 2 + v1 * t_decel;
		t_accel = (v1 + max_vel) / max_accel;
		s_accel = max_accel * t_accel * t_accel / 2 + v0 * t_accel;
		if (s < s_accel + s_decel) {
			// t = (s_accel + s_decel - s) / max_vel + t_accel + t_decel;
			return v0 - max_accel * time_step;
		}

		double t_to_v1 = std::fabs(v1 - v0) / max_accel;
		s_accel = max_accel * t_to_v1 * t_to_v1 / 2 + v0 * t_to_v1;

		// Case 3: accel, decel
		if (s_accel <= s) {
			return v0 + max_accel * time_step;
		}

		// Case 4: decel, accel
		return v0 - max_accel * time_step;	
	}

	void TestingRC::move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) {
		const Point &current_position = plr->position();
		const double current_orientation = plr->orientation();
		if (!initialized) {
			old_position = current_position;
			old_orientation = current_orientation;

			time_step = 1. / 30;
			max_angular_velocity_accel = M_PI;
			max_angular_velocity = 4 * M_PI;
			max_linear_velocity_accel = 2;
			max_linear_velocity = 10;
		}

		const Point &lin_vel = (current_position - old_position) / time_step;
		double da = angle_mod(new_orientation - current_orientation);
		const Point &d = (new_position - current_position).rotate(-current_orientation);
		old_position = current_position;
		old_orientation = current_orientation;

		Point tmp(get_velocity(d.x, lin_vel.x, 0, max_linear_velocity, max_linear_velocity_accel),
				get_velocity(d.y, lin_vel.y, 0, max_linear_velocity, max_linear_velocity_accel));

		linear_velocity = d;
		angular_velocity = da;
	}

	class TestingRCFactory : public RobotControllerFactory {
		public:
			TestingRCFactory() : RobotControllerFactory("Testing RC") {
			}

			RobotController::Ptr create_controller(Player::Ptr plr, bool, unsigned int) const {
				RobotController::Ptr p(new TestingRC(plr));
				return p;
			}
	};

	TestingRCFactory factory;

	RobotControllerFactory &TestingRC::get_factory() const {
		return factory;
	}
}

