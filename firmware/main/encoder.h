#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "io.h"


static inline int16_t read_encoder(uint8_t encoder_index) {
	uint16_t ret_val;
	ENCODER_LSB = encoder_index;
	ret_val = ENCODER_MSB;
	ret_val = (ret_val << 8) + ENCODER_LSB;
	return ret_val;
}
#endif
