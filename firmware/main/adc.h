#ifndef ADC_H
#define ADC_H

#include "io.h"
#include <stdint.h>

typedef enum {
	CHICKER,
	BATT_VOLT,
	LPS,
	BREAKBEAM,
} adc_t;

static inline uint16_t read_main_adc(adc_t adc_index) {
	uint16_t ret_val;
	outb(ADC_LSB, adc_index);
	ret_val = inb(ADC_MSB);
	ret_val = (ret_val << 8) | inb(ADC_LSB);
	return ret_val;
}

#endif

