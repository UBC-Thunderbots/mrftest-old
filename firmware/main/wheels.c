#include "wheels.h"
#include "adc.h"
#include "control.h"
#include "drive.h"
#include "encoder.h"
#include "motor.h"
#include "receive.h"
#include <rcc.h>
#include <registers/timer.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define THERMAL_TIME_CONSTANT 13.2f // seconds—EC45 datasheet
#define THERMAL_RESISTANCE 4.57f // kelvins per Watt—EC45 datasheet (winding to housing)
#define THERMAL_CAPACITANCE (THERMAL_TIME_CONSTANT / THERMAL_RESISTANCE) // joules per kelvin—estimated by binaryblade 2013-06-14
#define THERMAL_AMBIENT 40.0f // °C—empirically estimated based on motor casing heatsinking to chassis
#define THERMAL_MAX_TEMPERATURE 125.0f // °C—EC45 datasheet
#define THERMAL_MAX_ENERGY ((THERMAL_MAX_TEMPERATURE - THERMAL_AMBIENT) * THERMAL_CAPACITANCE) // joules
#define THERMAL_WARNING_START_TEMPERATURE (THERMAL_MAX_TEMPERATURE - 10.0f) // °C—chead
#define THERMAL_WARNING_START_ENERGY ((THERMAL_WARNING_START_TEMPERATURE - THERMAL_AMBIENT) * THERMAL_CAPACITANCE) // joules
#define THERMAL_WARNING_STOP_TEMPERATURE (THERMAL_WARNING_START_TEMPERATURE - 10.0f) // °C—chead
#define THERMAL_WARNING_STOP_ENERGY ((THERMAL_WARNING_STOP_TEMPERATURE - THERMAL_AMBIENT) * THERMAL_CAPACITANCE) // joules

#define PHASE_RESISTANCE 1.2f // ohms—EC45 datasheet
#define SWITCH_RESISTANCE 0.6f // ohms—L6234 datasheet

#define NUM_WHEELS 4U

static uint16_t last_data_serial = 0xFFFFU;
static float energy[NUM_WHEELS] = {};
static uint8_t hot;

static void set_nominal_drive(unsigned int index, motor_mode_t mode, int16_t drive, log_record_t *record) {
	// Fix negative numbers.
	if (drive < 0) {
		drive = -drive;
		if (mode == MOTOR_MODE_FORWARD) {
			mode = MOTOR_MODE_BACKWARD;
		}
	}

	// Fix out-of-range numbers.
	if (drive > 255) {
		drive = 255;
	}

	// Check temperature.
	bool critical_temp = energy[index] > THERMAL_MAX_ENERGY;

	// Drive the motor.
	if (!critical_temp) {
		motor_set(index, mode, (uint8_t) drive);
		if (record) {
			record->tick.wheels_drives[index] = mode == MOTOR_MODE_BACKWARD ? -drive : drive;
		}
	} else {
		motor_set(index, MOTOR_MODE_COAST, 0U);
		if (record) {
			record->tick.wheels_drives[index] = 0;
		}
	}

	// Update the thermal model.
	float added_energy;
	if (critical_temp || mode == MOTOR_MODE_COAST || mode == MOTOR_MODE_BRAKE) {
		added_energy = 0.0f;
	} else {
		float applied_delta_voltage = drive / 255.0f * adc_battery() - encoder_speed(index) * WHEELS_VOLTS_PER_ENCODER_COUNT;
		float current = applied_delta_voltage / (PHASE_RESISTANCE + SWITCH_RESISTANCE);
		float power = current * current * PHASE_RESISTANCE;
		added_energy = power / CONTROL_LOOP_HZ;
	}
	energy[index] = energy[index] + added_energy - (energy[index] / THERMAL_CAPACITANCE / THERMAL_RESISTANCE / CONTROL_LOOP_HZ);

	if (record) {
		record->tick.wheels_temperatures[index] = (uint8_t) (energy[index] / THERMAL_CAPACITANCE + THERMAL_AMBIENT);
	}
}

/**
 * \brief Updates the wheels.
 *
 * This function runs the control loop (if needed) and sends new power levels to the motor module.
 *
 * \param[in] drive the most recent drive packet that indicates how to control the wheels
 *
 * \param[out] record the log record whose wheel-related fields will be filled
 */
void wheels_tick(const drive_t *drive, log_record_t *record) {
	if (record) {
		record->tick.wheels_hall_sensors_failed = 0U;
		for (unsigned int i = 0U; i != NUM_WHEELS; ++i) {
			record->tick.wheels_setpoints[i] = drive->setpoints[i];
			record->tick.wheels_encoder_counts[i] = encoder_speed(i);
			if (motor_hall_stuck_low(i)) {
				record->tick.wheels_hall_sensors_failed |= 1U << (i * 2U);
			} else if (motor_hall_stuck_high(i)) {
				record->tick.wheels_hall_sensors_failed |= 2U << (i * 2U);
			}
		}
#warning check for optical encoder failures
		record->tick.wheels_encoders_failed = 0U;
	}

	if (drive->wheels_mode == WHEELS_MODE_CLOSED_LOOP) {
#warning check for optical encoder failures
		// If new setpoints were sent, deliver them to the controller.
		if (drive->data_serial != last_data_serial) {
			control_process_new_setpoints(drive->setpoints);
		}

		// Read out controller feedback values from optical encoders.
		int16_t feedback[NUM_WHEELS];
		for (unsigned int i = 0U; i != NUM_WHEELS; ++i) {
			feedback[i] = encoder_speed(i);
		}

		// Run the controller.
		int16_t pwm[NUM_WHEELS];
		control_tick(feedback, pwm);

		// Send the PWM values to the motors.
		for (unsigned int i = 0U; i != NUM_WHEELS; ++i) {
			set_nominal_drive(i, MOTOR_MODE_FORWARD, pwm[i], record);
		}
	} else {
		// Clear accumulated control data.
		control_clear();

		// Send the PWM values to the motors.
		motor_mode_t mmode;
		switch (drive->wheels_mode) {
			case WHEELS_MODE_BRAKE: mmode = MOTOR_MODE_BRAKE; break;
			case WHEELS_MODE_OPEN_LOOP: mmode = MOTOR_MODE_FORWARD; break;
			default: mmode = MOTOR_MODE_COAST; break;
		}
		for (unsigned int i = 0U; i != NUM_WHEELS; ++i) {
			set_nominal_drive(i, mmode, drive->setpoints[i], record);
		}
	}

	// Update the hot wheel bitmask.
	uint8_t new_hot = hot;
	for (unsigned int index = 0U; index != NUM_WHEELS; ++index) {
		if (energy[index] > THERMAL_WARNING_START_ENERGY) {
			new_hot |= 1U << index;
		} else if (energy[index] < THERMAL_WARNING_STOP_ENERGY) {
			new_hot &= ~(1U << index);
		}
	}
	__atomic_store_n(&hot, new_hot, __ATOMIC_RELAXED);
}

uint8_t wheels_hot(void) {
	return __atomic_load_n(&hot, __ATOMIC_RELAXED);
}

