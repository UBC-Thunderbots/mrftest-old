#ifndef DRIBBLER_H
#define DRIBBLER_H

#include <stdbool.h>
#include <stdint.h>

#define DRIBBLER_TICK_HZ 25U

#define DRIBBLER_THERMAL_TIME_CONSTANT_WINDING 1.97 // seconds—EC16 datasheet
#define DRIBBLER_THERMAL_RESISTANCE_WINDING 1.68 // kelvins per Watt—EC16 datasheet (winding to housing)
#define DRIBBLER_THERMAL_CAPACITANCE_WINDING (DRIBBLER_THERMAL_TIME_CONSTANT_WINDING / DRIBBLER_THERMAL_RESISTANCE_WINDING) // joules per kelvin

#define DRIBBLER_THERMAL_TIME_CONSTANT_HOUSING 240.0 // seconds—EC16 datasheet
#define DRIBBLER_THERMAL_RESISTANCE_HOUSING 16.3 // kelvins per Watt—EC16 datasheet (housing to ambient)
#define DRIBBLER_THERMAL_CAPACITANCE_HOUSING (DRIBBLER_THERMAL_TIME_CONSTANT_HOUSING / DRIBBLER_THERMAL_RESISTANCE_HOUSING) // joules per kelvin

#define DRIBBLER_THERMAL_AMBIENT 40.0 // °C—empirically estimated based on motor casing heatsinking to chassis
#define DRIBBLER_THERMAL_MAX_TEMPERATURE_WINDING 155.0 // °C—EC16 datasheet
#define DRIBBLER_THERMAL_MAX_ENERGY_WINDING ((DRIBBLER_THERMAL_MAX_TEMPERATURE_WINDING - DRIBBLER_THERMAL_AMBIENT) * DRIBBLER_THERMAL_CAPACITANCE_WINDING) // joules
#define DRIBBLER_THERMAL_WARNING_START_TEMPERATURE_WINDING (DRIBBLER_THERMAL_MAX_TEMPERATURE_WINDING - 15.0) // °C—chead
#define DRIBBLER_THERMAL_WARNING_START_ENERGY_WINDING ((DRIBBLER_THERMAL_WARNING_START_TEMPERATURE_WINDING - DRIBBLER_THERMAL_AMBIENT) * DRIBBLER_THERMAL_CAPACITANCE_WINDING) // joules
#define DRIBBLER_THERMAL_WARNING_STOP_TEMPERATURE_WINDING (DRIBBLER_THERMAL_WARNING_START_TEMPERATURE_WINDING - 15.0) // °C—chead
#define DRIBBLER_THERMAL_WARNING_STOP_ENERGY_WINDING ((DRIBBLER_THERMAL_WARNING_STOP_TEMPERATURE_WINDING - DRIBBLER_THERMAL_AMBIENT) * DRIBBLER_THERMAL_CAPACITANCE_WINDING) // joules

/**
 * \brief Whether or not the dribbler is requested to be operating.
 */
extern bool dribbler_enabled;

/**
 * \brief Whether or not the dribbler is requested to run at high speed.
 */
extern bool dribbler_fast;

/**
 * \brief The speed of the dribbler.
 */
extern uint16_t dribbler_speed;

/**
 * \brief The estimated thermal energy of the dribbler winding, in joules.
 */
extern float dribbler_winding_energy;

/**
 * \brief The estimated thermal energy of the dribbler housing, in joules.
 */
extern float dribbler_housing_energy;

/**
 * \brief Whether the dribbler is at warning temperature.
 *
 * Hysteresis is incorporated into this variable.
 */
extern bool dribbler_hot;

/**
 * \brief Updates the dribbler.
 *
 * \param battery the battery voltage
 *
 * \param sensor_failures a mask of sensor failures detected on the dribbler motor
 */
void dribbler_tick(float battery);

/**
 * \brief Reports pwm value of the dribbler.
 */
extern uint8_t dribbler_pwm;

#endif

