#ifndef MAC_H
#define MAC_H

#include <stdint.h>
#include <stdbool.h>

static inline uint64_t get_mac() {
	static bool is_init = false;
	static uint64_t ret_val = 0x0000000000000000;
	if(!is_init) {
		while(!DEVICE_ID_STATUS);
		ret_val = DEVICE_ID6;
		ret_val = ret_val << 8;
		ret_val |= DEVICE_ID5;
		ret_val = ret_val << 8;
		ret_val |= DEVICE_ID4;
		ret_val = ret_val << 8;
		ret_val |= DEVICE_ID3;
		ret_val = ret_val << 8;
		ret_val |= DEVICE_ID2;
		ret_val = ret_val << 8;
		ret_val |= DEVICE_ID1;
		ret_val = ret_val << 8;
		ret_val |= DEVICE_ID0;
		ret_val = ret_val << 8;
		is_init = true;
	}
	return ret_val;
}

#endif
