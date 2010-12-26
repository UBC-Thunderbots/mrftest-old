#include "buffers.h"
#include <stdint.h>

static __data uint8_t xbee_buffer1[111], xbee_buffer2[111], xbee_buffer3[111], xbee_buffer4[111], xbee_buffer5[111], xbee_buffer6[111];

__data void * __code const xbee_buffers[6] = {
	xbee_buffer1,
	xbee_buffer2,
	xbee_buffer3,
	xbee_buffer4,
	xbee_buffer5,
	xbee_buffer6,
};

__data xbee_rxpacket_t xbee_packets[6];

