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

static __data uint8_t dongle_proto_out_buffer1[64], dongle_proto_out_buffer2[64], dongle_proto_out_buffer3[64];
static __data uint8_t dongle_proto_out_buffer4[64], dongle_proto_out_buffer5[64], dongle_proto_out_buffer6[64];

__data void * __code const dongle_proto_out_buffers[6] = {
	dongle_proto_out_buffer1,
	dongle_proto_out_buffer2,
	dongle_proto_out_buffer3,
	dongle_proto_out_buffer4,
	dongle_proto_out_buffer5,
	dongle_proto_out_buffer6,
};

