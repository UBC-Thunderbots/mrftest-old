#ifndef WHEEL_CONTROLLER_H
#define WHEEL_CONTROLLER_H

#include <stdint.h>

int16_t wheel_controller_iter(int16_t setpoint, int16_t feedback);

#endif

