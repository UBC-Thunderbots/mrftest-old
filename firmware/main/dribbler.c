/**
 * \defgroup DRIBBLER Dribbler Functions
 *
 * \brief These functions handle overall operation of the dribbler, including PWM selection, torque limiting, and thermal modelling.
 *
 * @{
 */
#include "dribbler.h"
#include "adc.h"
#include "hall.h"
#include "motor.h"
#include "receive.h"

#define SPEED_CONSTANT 3760.0f // rpm per volt—EC16 datasheet
#define VOLTS_PER_RPM (1.0f / SPEED_CONSTANT) // volts per rpm
#define VOLTS_PER_RPT (VOLTS_PER_RPM * 60.0f * DRIBBLER_TICK_HZ) // volts per rpt, rpt=revolutions per tick
#define VOLTS_PER_SPEED_UNIT (VOLTS_PER_RPT / 6.0f) // volts per Hall edge
#define PHASE_RESISTANCE 0.403f // ohms—EC16 datasheet
#define SWITCH_RESISTANCE (0.019f*2.0f) // ohms—AO4882 datasheet

#define THERMAL_TIME_CONSTANT_WINDING 1.97f // seconds—EC16 datasheet
#define THERMAL_RESISTANCE_WINDING 1.68f // kelvins per Watt—EC16 datasheet (winding to housing)
#define THERMAL_CAPACITANCE_WINDING (THERMAL_TIME_CONSTANT_WINDING / THERMAL_RESISTANCE_WINDING) // joules per kelvin

#define THERMAL_TIME_CONSTANT_HOUSING 240.0f // seconds—EC16 datasheet
#define THERMAL_RESISTANCE_HOUSING 16.3f // kelvins per Watt—EC16 datasheet (housing to ambient)
#define THERMAL_CAPACITANCE_HOUSING (THERMAL_TIME_CONSTANT_HOUSING / THERMAL_RESISTANCE_HOUSING) // joules per kelvin

#define THERMAL_AMBIENT 40.0f // °C—empirically estimated based on motor casing heatsinking to chassis
#define THERMAL_MAX_TEMPERATURE_WINDING 155.0f // °C—EC16 datasheet
#define THERMAL_MAX_ENERGY_WINDING ((THERMAL_MAX_TEMPERATURE_WINDING - THERMAL_AMBIENT) * THERMAL_CAPACITANCE_WINDING) // joules
#define THERMAL_WARNING_START_TEMPERATURE_WINDING (THERMAL_MAX_TEMPERATURE_WINDING - 15.0f) // °C—chead
#define THERMAL_WARNING_START_ENERGY_WINDING ((THERMAL_WARNING_START_TEMPERATURE_WINDING - THERMAL_AMBIENT) * THERMAL_CAPACITANCE_WINDING) // joules
#define THERMAL_WARNING_STOP_TEMPERATURE_WINDING (THERMAL_WARNING_START_TEMPERATURE_WINDING - 15.0f) // °C—chead
#define THERMAL_WARNING_STOP_ENERGY_WINDING ((THERMAL_WARNING_STOP_TEMPERATURE_WINDING - THERMAL_AMBIENT) * THERMAL_CAPACITANCE_WINDING) // joules

static float winding_energy = 0.0f, housing_energy = 0.0f;
static bool hot = false;
static unsigned int temperature = 0U;

static void update_thermal_model(float added_winding_energy) {
	float energy_winding_to_housing = (winding_energy / THERMAL_CAPACITANCE_WINDING - housing_energy / THERMAL_CAPACITANCE_HOUSING) / THERMAL_RESISTANCE_WINDING / DRIBBLER_TICK_HZ;
	housing_energy = housing_energy + energy_winding_to_housing - housing_energy / THERMAL_CAPACITANCE_HOUSING / THERMAL_RESISTANCE_HOUSING / DRIBBLER_TICK_HZ;
	winding_energy = winding_energy - energy_winding_to_housing + added_winding_energy;
	if (winding_energy > THERMAL_WARNING_START_ENERGY_WINDING) {
		__atomic_store_n(&hot, true, __ATOMIC_RELAXED);
	} else if (winding_energy < THERMAL_WARNING_STOP_ENERGY_WINDING) {
		__atomic_store_n(&hot, false, __ATOMIC_RELAXED);
	}
	__atomic_store_n(&temperature, (unsigned int) (THERMAL_AMBIENT + winding_energy / THERMAL_CAPACITANCE_WINDING), __ATOMIC_RELAXED);
}

/**
 * \brief Updates the dribbler.
 *
 * \param[in] pwm the power level of the dribbler
 *
 * \param[out] record the log record whose dribbler-related fields will be updated
 */
void dribbler_tick(uint8_t pwm, log_record_t *record) {
	// Measure the dribbler speed.
	int16_t dribbler_speed = hall_speed(4U);

	// Do some log record filling.
	if (record) {
		record->tick.dribbler_ticked = 1U;
		record->tick.dribbler_speed = dribbler_speed;
	}

	// Decide whether to run or not.
	if (winding_energy < THERMAL_MAX_ENERGY_WINDING) {
		motor_set(4U, MOTOR_MODE_FORWARD, pwm);
		float battery = adc_battery();
		float back_emf = dribbler_speed * VOLTS_PER_SPEED_UNIT;
		float applied_voltage = battery * pwm / 255.0f;
		float delta_voltage = applied_voltage - back_emf;
		float current = delta_voltage / (PHASE_RESISTANCE + SWITCH_RESISTANCE);
		float power = current * current * PHASE_RESISTANCE;
		float energy = power / DRIBBLER_TICK_HZ;
		update_thermal_model(energy);

		if (record) {
			record->tick.dribbler_pwm = pwm;
		}
	} else {
		motor_set(4U, MOTOR_MODE_COAST, 0U);
		update_thermal_model(0.0f);
		if (record) {
			record->tick.dribbler_pwm = 0U;
		}
	}

	if (record) {
		record->tick.dribbler_temperature = (uint8_t) (winding_energy / THERMAL_CAPACITANCE_WINDING + THERMAL_AMBIENT);
	}
}

/**
 * \brief Checks whether the dribbler is hot.
 *
 * \retval true the dribbler is hot
 * \retval false the dribbler is not hot
 */
bool dribbler_hot(void) {
	return __atomic_load_n(&hot, __ATOMIC_RELAXED);
}

/**
 * \brief Returns the estimated temperature of the dribbler in °C.
 *
 * \return the dribbler temperature
 */
unsigned int dribbler_temperature(void) {
	return __atomic_load_n(&temperature, __ATOMIC_RELAXED);
}

/**
 * @}
 */

