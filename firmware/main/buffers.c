#include "buffers.h"

buffer_t buffers[2];
buffer_t *current_buffer = &buffers[0];

void buffers_swap(void) {
	current_buffer = current_buffer == &buffers[0] ? &buffers[1] : &buffers[0];
}

