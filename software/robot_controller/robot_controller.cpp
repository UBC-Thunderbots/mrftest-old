#include "robot_controller/robot_controller.h"

void robot_controller2::convert_to_wheels(const point &vel, double avel, int (&wheel_speeds)[4]) {
	static const double WHEEL_MATRIX[4][3] = {
		{-42.5995, 27.6645, 4.3175},
		{-35.9169, -35.9169, 4.3175},
		{35.9169, -35.9169, 4.3175},
		{42.5995, 27.6645, 4.3175}
	};
	const double input[3] = {vel.x, vel.y, avel};
	double output[4] = {0, 0, 0, 0};
	for (unsigned int row = 0; row < 4; ++row)
		for (unsigned int col = 0; col < 3; ++col)
			output[row] += WHEEL_MATRIX[row][col] * input[col];
	for (unsigned int row = 0; row < 4; ++row)
		wheel_speeds[row] = static_cast<int>(output[row]);
}

void robot_controller::move(const point &new_position, double new_orientation, int (&wheel_speeds)[4]) {
	point vel;
	double avel;
	move(new_position, new_orientation, vel, avel);
	convert_to_wheels(vel, avel, wheel_speeds);
}

