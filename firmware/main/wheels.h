#ifndef WHEELS_H
#define WHEELS_H

#include <stdint.h>

/**
 * \brief The modes the wheels can be in
 */
typedef enum {
	WHEEL_MODE_COAST,
	WHEEL_MODE_BRAKE,
	WHEEL_MODE_OPEN_LOOP,
	WHEEL_MODE_CLOSED_LOOP,
} wheel_mode_t;

/**
 * \brief The mode the wheels are in
 */
extern wheel_mode_t wheel_mode;

/**
 * \brief The setpoints for the wheels,
 * in quarters of a degree per five milliseconds (for controlled mode) or PWM duty cycle out of 255 (for uncontrolled)
 */
extern int16_t wheel_setpoint[4];

/**
 * \brief Runs the wheels
 */
void wheels_tick();

#endif

