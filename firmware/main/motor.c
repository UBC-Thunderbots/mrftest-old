#include "motor.h"
#include "icb.h"
#include "pins.h"
#include <rcc.h>
#include <string.h>
#include <registers/timer.h>

static uint8_t motor_packet[10U];
static uint8_t manual_commutation_patterns[5U] = { 0U, 0U, 0U, 0U, 0U };
static bool force_power = false;
static uint8_t stuck_halls[2U];

/**
 * \brief Initializes the motors.
 */
void motor_init(void) {
	// The FPGA powers up with all Hall sensors reading low momentarily, until the majority-detect filters flush through.
	// Clear out the stuck-low errors resulting from this.
	static uint8_t garbage[2U];
	icb_receive(ICB_COMMAND_MOTORS_GET_CLEAR_STUCK_HALLS, garbage, sizeof(garbage));
}

/**
 * \brief Stops all the motors.
 */
void motor_shutdown(void) {
	// Set them all to floating manual-commutation.
	for (unsigned int index = 0U; index < 5U; ++index) {
		motor_packet[index * 2U] = 0U;
		motor_set(index, MOTOR_MODE_MANUAL_COMMUTATION, 0U);
	}
	motor_tick();
}

/**
 * \brief Drives a motor.
 *
 * \param[in] motor_num the motor number, 0–3 for a wheel or 4 for the dribbler
 *
 * \param[in] mode the mode in which to drive the motor
 *
 * \param[in] pwm_level the PWM duty cycle to send
 */
void motor_set(unsigned int wheel_num, motor_mode_t mode, uint8_t pwm_level) {
	// Set the new mode, driving phases appropriately.
	switch (mode) {
		case MOTOR_MODE_MANUAL_COMMUTATION:
			motor_packet[wheel_num * 2U] = manual_commutation_patterns[wheel_num];
			break;

		case MOTOR_MODE_BRAKE:
			motor_packet[wheel_num * 2U] = 0b00101010;
			break;

		case MOTOR_MODE_FORWARD:
			motor_packet[wheel_num * 2U] = 0b10000000;
			break;

		case MOTOR_MODE_BACKWARD:
			motor_packet[wheel_num * 2U] = 0b11000000;
			break;
	}

	// Record PWM level.
	motor_packet[wheel_num * 2U + 1U] = pwm_level;
}

/**
 * \brief Sets the manual commutation pattern for a motor.
 *
 * \param[in] motor the index of the motor to adjust
 * \param[in] the 6-bit manual commutation pattern to apply
 */
void motor_set_manual_commutation_pattern(unsigned int motor, uint8_t pattern) {
	__atomic_store_n(&manual_commutation_patterns[motor], pattern & 0x3FU, __ATOMIC_RELAXED);
}

/**
 * \brief Forces on the motor power supply.
 */
void motor_force_power(void) {
	__atomic_store_n(&force_power, true, __ATOMIC_RELAXED);
}

/**
 * \brief Checks whether a motor’s Hall sensors are stuck low.
 *
 * \param[in] motor the index of the motor to check
 *
 * \retval true the Hall sensors were observed stuck low in the last tick period
 * \retval false the Hall sensors were not observed stuck low in the last tick period
 */
bool motor_hall_stuck_low(unsigned int motor) {
	return __atomic_load_n(&stuck_halls[0U], __ATOMIC_RELAXED) & (1U << motor);
}

/**
 * \brief Checks whether a motor’s Hall sensors are stuck high.
 *
 * \param[in] motor the index of the motor to check
 *
 * \retval true the Hall sensors were observed stuck high in the last tick period
 * \retval false the Hall sensors were not observed stuck high in the last tick period
 */
bool motor_hall_stuck_high(unsigned int motor) {
	return __atomic_load_n(&stuck_halls[1U], __ATOMIC_RELAXED) & (1U << motor);
}

/**
 * \brief Clears accumulated data of whether motor Hall sensors are stuck.
 */
void motor_hall_stuck_clear(void) {
	__atomic_store_n(&stuck_halls[0U], 0U, __ATOMIC_RELAXED);
	__atomic_store_n(&stuck_halls[1U], 0U, __ATOMIC_RELAXED);
}

/**
 * \brief Sends the drive data to the motors.
 */
void motor_tick(void) {
	// Send current motor commands.
	icb_send(ICB_COMMAND_MOTORS_SET, motor_packet, sizeof(motor_packet));

	// Enable HV power rail when forced or when any motor enabled.
	// Never disable it once enabled.
	bool power = __atomic_load_n(&force_power, __ATOMIC_RELAXED);
	for (unsigned int i = 0U; i != 5U; ++i) {
		power = power || (motor_packet[i * 2U] != 0U || motor_packet[i * 2U + 1] != 0U);
	}
	if (power) {
		gpio_set_output(PIN_POWER_HV, power);
	}

	// Read back stuck Hall sensors.
	static uint8_t new_stuck[2U];
	icb_receive(ICB_COMMAND_MOTORS_GET_CLEAR_STUCK_HALLS, new_stuck, sizeof(new_stuck));
	for (unsigned int i = 0U; i != sizeof(new_stuck) / sizeof(*new_stuck); ++i) {
		__atomic_or_fetch(&stuck_halls[i], new_stuck[i], __ATOMIC_RELAXED);
	}
}

