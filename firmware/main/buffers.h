#ifndef BUFFERS_H
#define BUFFERS_H

#include <stdint.h>

typedef struct {
	uint16_t epoch;
	uint32_t ticks;
	int16_t breakbeam_diff;
	uint16_t adc_channels[8];
} tick_info_t;

typedef struct {
	uint8_t packets[3][128];
	union {
		tick_info_t tick;
		uint8_t pad[128];
	};
} buffer_t;

extern buffer_t buffers[2];
extern buffer_t *current_buffer;

void buffers_swap(void);

#endif

