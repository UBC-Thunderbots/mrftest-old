#include "buffers.h"

typedef int TICK_INFO_T_IS_TOO_FAT[sizeof(buffer_t) == 512 ? 1 : -1];

buffer_t buffers[2];
buffer_t *current_buffer = &buffers[0];
uint8_t *next_packet_buffer = buffers[0].packets[0];
static uint8_t next_packet_buffer_index = 0;

void buffers_swap(void) {
	current_buffer = current_buffer == &buffers[0] ? &buffers[1] : &buffers[0];
	next_packet_buffer = current_buffer->packets[0];
	next_packet_buffer_index = 0;
}

void buffers_push_packet(void) {
	if (next_packet_buffer_index < 2) {
		++next_packet_buffer_index;
		next_packet_buffer = current_buffer->packets[next_packet_buffer_index];
	}
}

