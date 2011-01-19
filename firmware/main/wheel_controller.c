#include "wheel_controller.h"

int16_t wheel_controller_iter(int16_t setpoint, int16_t feedback) {
	int16_t diff = setpoint - feedback;
	return 1 * diff;
}

