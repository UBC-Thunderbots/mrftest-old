#include <math.h>

#include "constants.h"
#include "wheel.h"
#include "pwmpins.h"
#include "iopins.h"
#include "filter.h"

extern double motor_battery_voltage;

// For better code generation when pin is dynamic rather than compile-time constant.
static void write_pin(uint8_t pin, uint8_t value) {
	iopin_write(pin, value);
}

void wheel_init(struct wheel *w, uint8_t counter_oe_pin, uint8_t motor_a_pin, uint8_t motor_b_pin, uint8_t motor_pwm_pin, const double *scale_factors, const double *rpm_filter_a, const double *rpm_filter_b, const double *controller_a, const double *controller_b,const double* plant_a,const double* plant_b) {
	filter_init(&w->rpm_filter, rpm_filter_a, rpm_filter_b);
	filter_init(&w->controller, controller_a, controller_b);
	filter_init(&w->plant, plant_a, plant_b);
	w->counter_oe_pin = counter_oe_pin;
	w->motor_a_pin = motor_a_pin;
	w->motor_b_pin = motor_b_pin;
	w->motor_pwm_pin = motor_pwm_pin;
	w->scale_factors = scale_factors;
	w->max_volt_diff = sqrt(WHEEL_MAX_POWER * MOTOR_RESISTANCE);
	w->plantPrediction = 0;
}

void wheel_clear(struct wheel *w) {
	filter_clear(&w->rpm_filter);
	filter_clear(&w->controller);
	filter_clear(&w->plant);
	w->cur_count = 0;
	w->last_count = 0;
	w->plantPrediction = 0;
}

void wheel_update_rpm(struct wheel *w) {
	write_pin(w->counter_oe_pin, 0);
	w->last_count = w->cur_count;
	w->cur_count = ioport_read(IOPORT_COUNTER_DATA);
	write_pin(w->counter_oe_pin, 1);
}

void wheel_update_drive(struct wheel *w, double vx, double vy, double vt) {
	double setpoint, cur_rpm, power;
	uint16_t max_pwm_dynamic, pwm_level;

	// Compute motor percentage setpoint.
	setpoint = vx * w->scale_factors[0] + vy * w->scale_factors[1] + vt * w->scale_factors[2];

	// Read counter and determine current RPM.
	cur_rpm = filter_process(&w->rpm_filter, ENCODER_COUNTS_TO_RPM / LOOP_TIME * (int8_t) (w->cur_count - w->last_count));

	// Pass the error in motor percentage through the controller.
#if W_CONTROLLER_ENABLED
	power = filter_process(&w->controller, setpoint - cur_rpm / MOTOR_MAX_RPM + w->plantPrediction);
#else
	power = setpoint * MANUAL_ACTUATOR_MOTOR_SCALE;
#endif

	// Drive the motor.
	if (power > 0.0) {
		write_pin(w->motor_a_pin, 1);
		write_pin(w->motor_b_pin, 0);
	} else {
		write_pin(w->motor_a_pin, 0);
		write_pin(w->motor_b_pin, 1);
	}
	pwm_level = fabs(power) * 1023.0 + 0.5;

	// Cap it based on a static cap on voltage and also a cap based on current measurements.
	max_pwm_dynamic = (fabs(cur_rpm / VOLTAGE_TO_RPM) + w->max_volt_diff) / motor_battery_voltage * 1023.0 + 0.5;
	if (max_pwm_dynamic > MOTOR_CAP)
		max_pwm_dynamic = MOTOR_CAP;
	if (pwm_level > max_pwm_dynamic)
		pwm_level = max_pwm_dynamic;
		
	w->plantPrediction = filter_process(&w->plant, pwm_level / 1023.0 * (power > 0) ? 1 : -1);
	
	pwm_write(w->motor_pwm_pin, pwm_level);
}

