#include "wheels.h"
#include "control.h"
#include "io.h"
#include "motor.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

wheels_mode_t wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
wheels_setpoints_t wheels_setpoints;
int16_t wheels_encoder_counts[4] = { 0, 0, 0, 0 };
int16_t wheels_drives[4] = { 0, 0, 0, 0 };
float wheels_energy[4] = { 0, 0, 0, 0 };
uint8_t wheels_hot = 0;

static float update_thermal_model(float old, float new) {
	return old + new - (old / WHEEL_THERMAL_CAPACITANCE / WHEEL_THERMAL_RESISTANCE / CONTROL_LOOP_HZ);
}

static void update_wheels_hot(void) {
	for (uint8_t i = 0, bm = 1; i != 4; ++i, bm <<= 1) {
		if (wheels_energy[i] > WHEEL_THERMAL_WARNING_START_ENERGY) {
			wheels_hot |= bm;
		} else if (wheels_energy[i] < WHEEL_THERMAL_WARNING_STOP_ENERGY) {
			wheels_hot &= ~bm;
		}
	}
}

void wheels_tick(float battery, const uint8_t sensor_failures[4]) {
	// Read optical encoders.
	for (unsigned int i = 0; i < 4; ++i) {
		wheels_encoder_counts[i] = motor_speed(i);
	}

	switch (wheels_mode) {
		case WHEELS_MODE_MANUAL_COMMUTATION:
		case WHEELS_MODE_BRAKE:
			{
				// The controller is not used here and should be cleared.
				control_clear();

				// In these modes, we send the PWM duty cycle given in the setpoint.
				// Safety interlocks normally prevent it from ever appearing outside the chip, but if interlocks are overridden, manual commutation can send PWM to a phase.
				motor_mode_t mmode = wheels_mode == WHEELS_MODE_MANUAL_COMMUTATION ? MOTOR_MODE_MANUAL_COMMUTATION : MOTOR_MODE_BRAKE;
				for (unsigned int i = 0; i < 4; ++i) {
					motor_set(i, mmode, wheels_setpoints.wheels[i]);
				}

				// For reporting purposes, the drive levels are considered to be zero here.
				memset(wheels_drives, 0, sizeof(wheels_drives));

				// Update thermal models.
				for (unsigned int i = 0; i < 4; ++i) {
					wheels_energy[i] = update_thermal_model(wheels_energy[i], 0);
				}
				update_wheels_hot();
			}
			break;

		case WHEELS_MODE_OPEN_LOOP:
		case WHEELS_MODE_CLOSED_LOOP:
			{
				// Compute drive level, either by running the controller or by clearing the controller and taking the setpoints directly.
				if (wheels_mode == WHEELS_MODE_OPEN_LOOP) {
					control_clear();
					memcpy(wheels_drives, wheels_setpoints.wheels, sizeof(wheels_drives));
				} else {
					control_tick(battery);
				}

				// Clamp all drive levels to ±255.
				for (unsigned int i = 0; i < 4; ++i) {
					if (wheels_drives[i] < -255) {
						wheels_drives[i] = -255;
					} else if (wheels_drives[i] > 255) {
						wheels_drives[i] = 255;
					}
				}

				// Construct a bitmask of which motors we will drive (by default, all of them).
				unsigned int drive_mask = 0x0F;

				// Safety interlock: if an optical encoder fails, we must coast that motor as there is no provably safe duty cycle that will avoid over-current.
				if (IO_SYSCTL.csr.software_interlock) {
					for (unsigned int i = 0; i < 4; ++i) {
						if (sensor_failures[i] & 4) {
							drive_mask &= ~(1 << i);
						}
					}
				}

				// Safety interlock: if a motor is too hot, coast it until it cools down.
				for (unsigned int i = 0; i < 4; ++i) {
					unsigned int bm = 1 << i;
					if (((drive_mask & bm) && wheels_energy[i] < WHEEL_THERMAL_MAX_ENERGY) || !IO_SYSCTL.csr.software_interlock) {
						float applied_delta_voltage = wheels_drives[i] / 255.0 * battery - wheels_encoder_counts[i] * WHEEL_VOLTS_PER_ENCODER_COUNT;
						float current = applied_delta_voltage / (WHEEL_PHASE_RESISTANCE + WHEEL_SWITCH_RESISTANCE);
						float power = current * current * WHEEL_PHASE_RESISTANCE;
						float energy = power / CONTROL_LOOP_HZ;
						wheels_energy[i] = update_thermal_model(wheels_energy[i], energy);
					} else {
						drive_mask &= ~bm;
						wheels_energy[i] = update_thermal_model(wheels_energy[i], 0);
					}
				}
				update_wheels_hot();

				// Drive the motors.
				for (unsigned int i = 0; i < 4; ++i) {
					if (drive_mask & 1) {
						if (wheels_drives[i] >= 0) {
							motor_set(i, MOTOR_MODE_FORWARD, wheels_drives[i]);
						} else {
							motor_set(i, MOTOR_MODE_BACKWARD, -wheels_drives[i]);
						}
					} else {
						motor_set(i, MOTOR_MODE_MANUAL_COMMUTATION, 0);
					}
					drive_mask >>= 1;
				}
			}
			break;
	}
}

