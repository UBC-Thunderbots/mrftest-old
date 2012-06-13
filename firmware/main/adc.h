#ifndef ADC_H
#define ADC_H

#include "io.h"
#include <stdint.h>

typedef enum {
	THERMISTOR,
	BREAKBEAM,
	LPS,
	BATT_VOLT
} adc_t;

static inline uint16_t read_main_adc(adc_t adc_index) {
	uint16_t ret_val;
	outb(ADC_LSB, adc_index);
	ret_val = inb(ADC_MSB);
	ret_val = (ret_val << 8) | inb(ADC_LSB);
	return ret_val;
}

static inline uint16_t read_chicker_adc() {
	uint16_t ret_val;
	outb(CHICKER_ADC_LSB, 0x55);
	ret_val = inb(CHICKER_ADC_MSB);
	ret_val = (ret_val << 8) | inb(CHICKER_ADC_LSB);
	return ret_val;
}

#endif

