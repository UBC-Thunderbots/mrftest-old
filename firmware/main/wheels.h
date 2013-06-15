#ifndef WHEELS_H
#define WHEELS_H

#include <stdint.h>

#define WHEEL_THERMAL_TIME_CONSTANT 13.2 // seconds—EC45 datasheet
#define WHEEL_THERMAL_RESISTANCE 4.57 // kelvins per Watt—EC45 datasheet (winding to housing)
#define WHEEL_THERMAL_CAPACITANCE (WHEEL_THERMAL_TIME_CONSTANT / WHEEL_THERMAL_RESISTANCE) // joules per kelvin—estimated by binaryblade 2013-06-14
#define WHEEL_THERMAL_AMBIENT 40.0 // °C—empirically estimated based on motor casing heatsinking to chassis
#define WHEEL_THERMAL_MAX_TEMPERATURE 125.0 // °C—EC45 datasheet
#define WHEEL_THERMAL_MAX_ENERGY ((WHEEL_THERMAL_MAX_TEMPERATURE - WHEEL_THERMAL_AMBIENT) * WHEEL_THERMAL_CAPACITANCE)

#define WHEEL_SPEED_CONSTANT 374.0 // rpm per volt—EC45 datasheet
#define WHEEL_VOLTS_PER_RPM (1.0 / WHEEL_SPEED_CONSTANT) // volts per rpm
#define WHEEL_VOLTS_PER_RPT (WHEEL_VOLTS_PER_RPM / 60.0 / CONTROL_LOOP_HZ) // volts per rpt, rpt=revolutions per tick
#define WHEEL_VOLTS_PER_ENCODER_COUNT (WHEEL_VOLTS_PER_RPT / 1440.0) // volts per encoder count
#define WHEEL_PHASE_RESISTANCE 1.2 // ohms—EC45 datasheet
#define WHEEL_SWITCH_RESISTANCE 0.6 // ohms—L6234 datasheet

/**
 * \brief The modes the wheels can be in.
 */
typedef enum {
	WHEELS_MODE_MANUAL_COMMUTATION,
	WHEELS_MODE_BRAKE,
	WHEELS_MODE_OPEN_LOOP,
	WHEELS_MODE_CLOSED_LOOP,
} wheels_mode_t;

/**
 * \brief The type of a collection of setpoints.
 */
typedef union {
	int16_t wheels[4];
	float robot[3];
} wheels_setpoints_t;

/**
 * \brief The current mode of the wheels.
 */
extern wheels_mode_t wheels_mode;

/**
 * \brief The wheel speed or robot velocity setpoints currently applying.
 */
extern wheels_setpoints_t wheels_setpoints;

/**
 * \brief The most recently read wheel optical encoder counts.
 */
extern int16_t wheels_encoder_counts[4];

/**
 * \brief The most recently calculated drive levels to send to the wheel motors.
 */
extern int16_t wheels_drives[4];

/**
 * \brief The estimated thermal energy of the wheel motor windings, in joules.
 */
extern float wheels_energy[4];

/**
 * \brief Updates the wheels.
 *
 * This function updates \ref wheel_encoder_counts as well as running the control loop (if needed) and sending new power levels to the wheel motors.
 *
 * \param battery the battery voltage
 */
void wheels_tick(float battery);

#endif

