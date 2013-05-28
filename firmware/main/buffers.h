#ifndef BUFFERS_H
#define BUFFERS_H

#include <stdint.h>

typedef struct {
	uint16_t epoch;
	uint32_t ticks;
	int16_t breakbeam_diff;
	uint16_t adc_channels[8];
	int16_t encoder_counts[4];
	float setpoint[4];
	uint8_t motor_directions;
	uint8_t motor_drives[5];
	uint8_t encoders_failed;
	uint8_t wheel_hall_sensors_failed;
	uint8_t dribbler_hall_sensors_failed;
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
extern uint8_t *next_packet_buffer;

void buffers_swap(void);
void buffers_push_packet(void);

#endif

