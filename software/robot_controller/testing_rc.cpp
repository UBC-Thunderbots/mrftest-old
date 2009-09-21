#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player.h"
#include <cmath>

namespace {
	//
	// The robot controller.
	//
	class testing_rc : public virtual robot_controller {
		public:
			//
			// Constructs a new controller.
			//
			testing_rc(player::ptr player);

			//
			// Constructs a new controller.
			//
			// Parameters:
			//  target_position
			//   location the player wants to be in world coordinate
			//
			//  target_orientation
			//   direction the player wants to have
			//
			virtual void move(const point &target_position, double target_orientation);		

		private:
			double get_velocity(double d, double v0, double v1, double max_vel, double max_accel);

			point old_position;
			double old_orientation;

			double time_step;

			double max_angular_velocity;
			double max_angular_velocity_accel;

			double max_linear_velocity;
			double max_linear_velocity_accel;
	};

	testing_rc::testing_rc(player::ptr player) : robot_controller(player) {
		old_position = robot->position();
		old_orientation = robot->orientation();

		time_step = 1. / 30;
		max_angular_velocity_accel = PI;
		max_angular_velocity = 4 * PI;
		max_linear_velocity_accel = 2;
		max_linear_velocity = 10;
	}

	double testing_rc::get_velocity(double s, double v0, double v1, double max_vel, double max_accel) {
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

	void testing_rc::move(const point &tar_pos, double tar_ori) {
		const point &pos = robot->position();
		double ori = robot->orientation();
		const point &lin_vel = (pos - old_position) / time_step;
		double ang_vel = (ori - old_orientation) / time_step;
		double da = angle_mod(tar_ori - ori);
		const point &d = rotate((tar_pos - pos), -ori);
		old_position = pos;
		old_orientation = ori;	

		point tmp(get_velocity(d.real(), lin_vel.real(), 0, max_linear_velocity, max_linear_velocity_accel),
				get_velocity(d.imag(), lin_vel.imag(), 0, max_linear_velocity, max_linear_velocity_accel));
		double tmp_ang = get_velocity(da, ang_vel, 0, max_angular_velocity, max_angular_velocity_accel);

		robot->move(d, da);
	}

	class testing_rc_factory : public virtual robot_controller_factory {
		public:
			testing_rc_factory() : robot_controller_factory("Testing RC") {
			}

			virtual robot_controller::ptr create_controller(player::ptr bot) {
				robot_controller::ptr p(new testing_rc(bot));
				return p;
			}
	};

	testing_rc_factory factory;
}

