#include "util/thermal.h"
#include <cmath>

double adc_voltage_to_board_temp(double voltage) {
	// For V being ADC voltage and R being thermistor voltage:
	// V = 3.3 / (10,000 + R) * R
	// 10,000 V + VR = 3.3 R
	// (3.3 - V) R = 10,000 V
	// R = 10,000 V / (3.3 - V)
	double thermistor_resistance = 10000 * voltage / (3.3 - voltage);

	// Magic math from binaryblade
	double ltemp = std::log(thermistor_resistance);
	double temperature = 1.6648 * ltemp * ltemp - 61.3664 * ltemp + 510.18;

	return temperature;
}

