#include "dribbler.h"
#include "control.h"
#include "io.h"
#include "motor.h"
#include "power.h"

#define DRIBBLER_TICK_HZ 25U
#define CONTROL_TICKS_PER_DRIBBLER_TICK (CONTROL_LOOP_HZ / DRIBBLER_TICK_HZ)

#define THERMAL_TIME_CONSTANT_WINDING 1.97 // seconds—EC16 datasheet
#define THERMAL_RESISTANCE_WINDING 1.68 // kelvins per Watt—EC16 datasheet (winding to housing)
#define THERMAL_CAPACITANCE_WINDING (THERMAL_TIME_CONSTANT_WINDING / THERMAL_RESISTANCE_WINDING) // joules per kelvin

#define THERMAL_TIME_CONSTANT_HOUSING 240.0 // seconds—EC16 datasheet
#define THERMAL_RESISTANCE_HOUSING 16.3 // kelvins per Watt—EC16 datasheet (housing to ambient)
#define THERMAL_CAPACITANCE_HOUSING (THERMAL_TIME_CONSTANT_HOUSING / THERMAL_RESISTANCE_HOUSING) // joules per kelvin

#define THERMAL_AMBIENT 40.0 // °C—empirically estimated based on motor casing heatsinking to chassis
#define THERMAL_MAX_TEMPERATURE_WINDING 155.0 // °C—EC16 datasheet
#define THERMAL_MAX_ENERGY_WINDING ((THERMAL_MAX_TEMPERATURE_WINDING - THERMAL_AMBIENT) * THERMAL_CAPACITANCE_WINDING)

#define SPEED_CONSTANT 3760.0 // rpm per volt—EC16 datasheet
#define VOLTS_PER_RPM (1.0 / SPEED_CONSTANT) // volts per rpm
#define VOLTS_PER_RPT (VOLTS_PER_RPM * 60.0 * DRIBBLER_TICK_HZ) // volts per rpt, rpt=revolutions per tick
#define VOLTS_PER_SPEED_UNIT (VOLTS_PER_RPT / 6.0) // volts per Hall edge
#define PHASE_RESISTANCE 0.403 // ohms—EC16 datasheet
#define SWITCH_RESISTANCE 0.6 // ohms—L6234 datasheet

#define TARGET_PWM 180
#define MAX_DELTA_PWM 50

static uint8_t tick_count = 0;
bool dribbler_enabled = false;
uint8_t dribbler_speed = 0;
float dribbler_winding_energy = 0, dribbler_housing_energy = 0;

static void update_thermal_model(float added_winding_energy) {
	float energy_winding_to_housing = (dribbler_winding_energy / THERMAL_CAPACITANCE_WINDING - dribbler_housing_energy / THERMAL_CAPACITANCE_HOUSING) / THERMAL_RESISTANCE_WINDING / DRIBBLER_TICK_HZ;
	dribbler_housing_energy = dribbler_housing_energy + energy_winding_to_housing - dribbler_housing_energy / THERMAL_CAPACITANCE_HOUSING / THERMAL_RESISTANCE_HOUSING / DRIBBLER_TICK_HZ;
	dribbler_winding_energy = dribbler_winding_energy - energy_winding_to_housing + added_winding_energy;
}

void dribbler_tick(float battery) {
	// Run the controller only at the proper frequency.
	if (++tick_count != CONTROL_TICKS_PER_DRIBBLER_TICK) {
		return;
	}
	tick_count = 0;

	// Measure the dribbler speed.
	DRIBBLER_SPEED = 0;
	dribbler_speed = DRIBBLER_SPEED;

	// Decide whether to run or not.
	if ((dribbler_winding_energy < THERMAL_MAX_ENERGY_WINDING || interlocks_overridden()) && dribbler_enabled) {
		float back_emf = dribbler_speed * VOLTS_PER_SPEED_UNIT;
		uint16_t back_emf_pwm = (uint16_t) (back_emf / battery * 255.0);
		uint8_t min_pwm = back_emf_pwm <= MAX_DELTA_PWM ? 0 : (uint8_t) (back_emf_pwm - MAX_DELTA_PWM);
		uint8_t max_pwm = back_emf_pwm >= 255 - MAX_DELTA_PWM ? 255 : (uint8_t) (back_emf_pwm + MAX_DELTA_PWM);
		uint8_t pwm = TARGET_PWM;
		if (pwm > max_pwm) {
			pwm = max_pwm;
		} else if (pwm < min_pwm) {
			pwm = min_pwm;
		}
		motor_set(4, MOTOR_MODE_FORWARD, pwm);
		float applied_voltage = battery * pwm / 255.0;
		float delta_voltage = applied_voltage - back_emf;
		float current = delta_voltage / (PHASE_RESISTANCE + SWITCH_RESISTANCE);
		float power = current * current * PHASE_RESISTANCE;
		float energy = power / DRIBBLER_TICK_HZ;
		update_thermal_model(energy);
	} else {
		motor_set(4, MOTOR_MODE_MANUAL_COMMUTATION, 0);
		update_thermal_model(0);
	}
}

