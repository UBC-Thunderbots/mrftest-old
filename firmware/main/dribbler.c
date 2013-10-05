#include "dribbler.h"
#include "control.h"
#include "io.h"
#include "motor.h"

#define CONTROL_TICKS_PER_DRIBBLER_TICK (CONTROL_LOOP_HZ / DRIBBLER_TICK_HZ)

#define SPEED_CONSTANT 3760.0 // rpm per volt—EC16 datasheet
#define VOLTS_PER_RPM (1.0 / SPEED_CONSTANT) // volts per rpm
#define VOLTS_PER_RPT (VOLTS_PER_RPM * 60.0 * DRIBBLER_TICK_HZ) // volts per rpt, rpt=revolutions per tick
#define VOLTS_PER_SPEED_UNIT (VOLTS_PER_RPT / 6.0) // volts per Hall edge
#define PHASE_RESISTANCE 0.403 // ohms—EC16 datasheet
#define SWITCH_RESISTANCE 0.6 // ohms—L6234 datasheet

#define TARGET_PWM_FAST 180
#define TARGET_PWM_SLOW 80
#define MAX_DELTA_PWM 255

static uint8_t tick_count = 0;
bool dribbler_enabled = false;
bool dribbler_fast = true;
uint16_t dribbler_speed = 0;
uint8_t dribbler_pwm = 0;
float dribbler_winding_energy = 0, dribbler_housing_energy = 0;
bool dribbler_hot = false;

static void update_thermal_model(float added_winding_energy) {
	float energy_winding_to_housing = (dribbler_winding_energy / DRIBBLER_THERMAL_CAPACITANCE_WINDING - dribbler_housing_energy / DRIBBLER_THERMAL_CAPACITANCE_HOUSING) / DRIBBLER_THERMAL_RESISTANCE_WINDING / DRIBBLER_TICK_HZ;
	dribbler_housing_energy = dribbler_housing_energy + energy_winding_to_housing - dribbler_housing_energy / DRIBBLER_THERMAL_CAPACITANCE_HOUSING / DRIBBLER_THERMAL_RESISTANCE_HOUSING / DRIBBLER_TICK_HZ;
	dribbler_winding_energy = dribbler_winding_energy - energy_winding_to_housing + added_winding_energy;
	if (dribbler_winding_energy > DRIBBLER_THERMAL_WARNING_START_ENERGY_WINDING) {
		dribbler_hot = true;
	} else if (dribbler_winding_energy < DRIBBLER_THERMAL_WARNING_STOP_ENERGY_WINDING) {
		dribbler_hot = false;
	}
}

void dribbler_tick(float battery) {
	// Run the controller only at the proper frequency.
	if (++tick_count != CONTROL_TICKS_PER_DRIBBLER_TICK) {
		return;
	}
	tick_count = 0;

	// Measure the dribbler speed.
	dribbler_speed = motor_speed(4);

	// Decide whether to run or not.
	if ((dribbler_winding_energy < DRIBBLER_THERMAL_MAX_ENERGY_WINDING || !IO_SYSCTL.csr.software_interlock) && dribbler_enabled) {
		float back_emf = dribbler_speed * VOLTS_PER_SPEED_UNIT;
		uint16_t back_emf_pwm = (uint16_t) (back_emf / battery * 255.0);
		uint8_t min_pwm = back_emf_pwm <= MAX_DELTA_PWM ? 0 : (uint8_t) (back_emf_pwm - MAX_DELTA_PWM);
		uint8_t max_pwm = back_emf_pwm >= 255 - MAX_DELTA_PWM ? 255 : (uint8_t) (back_emf_pwm + MAX_DELTA_PWM);
		uint8_t pwm = dribbler_fast ? TARGET_PWM_FAST : TARGET_PWM_SLOW;
		if (pwm > max_pwm) {
			pwm = max_pwm;
		} else if (pwm < min_pwm) {
			pwm = min_pwm;
		}

		dribbler_pwm = pwm;

		motor_set(4, MOTOR_MODE_FORWARD, pwm);
		float applied_voltage = battery * pwm / 255.0;
		float delta_voltage = applied_voltage - back_emf;
		float current = delta_voltage / (PHASE_RESISTANCE + SWITCH_RESISTANCE);
		float power = current * current * PHASE_RESISTANCE;
		float energy = power / DRIBBLER_TICK_HZ;
		update_thermal_model(energy);
	} else {
		dribbler_pwm = 0;
		motor_set(4, MOTOR_MODE_MANUAL_COMMUTATION, 0);
		update_thermal_model(0);
	}
}

