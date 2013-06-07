#ifndef ENCODER_H
#define ENCODER_H

#include "io.h"
#include <stdint.h>

/**
 * \brief Reads and clears the count on an optical encoder.
 *
 * \param encoder_index the index of the optical encoder to read, from 0 to 3
 *
 * \return the distance the encoder has turned since the last time it was read
 */
static inline int16_t read_encoder(uint8_t encoder_index) {
	ENCODER_DATA = encoder_index;
	uint16_t ret_val = ENCODER_DATA;
	ret_val <<= 8;
	ret_val |= ENCODER_DATA;
	return ret_val;
}

#endif

