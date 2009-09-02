#include "robot_controller_test/player.h"
#include "geom/angle.h"
#include <cmath>



rc_test_player::rc_test_player(point position, double orientation, point linear_velocity, double angular_velocity) {
	time_step = 1. / 30;
	sub_steps = 100;
	dt = time_step / sub_steps;
	max_angular_velocity = 4 * PI;
	max_angular_velocity_accel = PI;
	max_linear_velocity = 10;
	max_linear_velocity_accel = 2;
	pos = position;
	ori = orientation;
	lin_vel = linear_velocity;
	ang_vel = angular_velocity;
}



point rc_test_player::position() const {
	return pos;
}



double rc_test_player::orientation() const {
	return ori;
}



point rc_test_player::linear_velocity() const {
	return lin_vel;
}



double rc_test_player::angular_velocity() const {
	return ang_vel;
}



void rc_test_player::move(const point &linear_velocity, double angular_velocity) {
	for (int i = 0; i < sub_steps; ++i) {
		/* angle acceleration limit */
		double da = angular_velocity - ang_vel;
		if (da > max_angular_velocity_accel * dt) da = max_angular_velocity_accel * dt;
		else if (da < -max_angular_velocity_accel * dt) da = -max_angular_velocity_accel * dt;

		/* angle velocity limit */
		ang_vel += da;
		if (ang_vel > max_angular_velocity) ang_vel = max_angular_velocity;
		else if (ang_vel < -max_angular_velocity) ang_vel = -max_angular_velocity;

		/* convert robot-relative coordinates to world coordinate */
		const point &wlv = linear_velocity * point(std::cos(ori), std::sin(ori));

		/* velocity acceleration limit */
		point dv = wlv - lin_vel;
		if (std::abs(dv) > max_linear_velocity_accel * dt)
			dv *= (max_linear_velocity_accel * dt) / std::abs(dv);

		/* velocity limit */
		lin_vel += dv;
		if (std::abs(lin_vel) > max_linear_velocity)
			lin_vel *= (max_linear_velocity) / std::abs(lin_vel);

		/* update */
		ori = angle_mod(ori + ang_vel * dt);
		pos += lin_vel * dt;
	}
}
