#ifndef UTIL_THERMAL_H
#define UTIL_THERMAL_H

/**
 * \brief Converts a voltage to a temperature measured by the robot’s on-board thermistor.
 *
 * \param[in] voltage the voltage, in volts
 *
 * \return the temperature, in °C
 */
double adc_voltage_to_board_temp(double voltage);

#endif

