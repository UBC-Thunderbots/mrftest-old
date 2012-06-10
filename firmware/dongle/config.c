#include "config.h"
#include "stdint.h"

struct config config = {
	.channel = UINT8_C(0x0B),
	.symbol_rate = UINT8_C(0x00),
	.pan_id = UINT16_C(0x0000),
	.mac_address = UINT64_C(0x0000000000000000),
};
