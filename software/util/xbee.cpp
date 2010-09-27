#include "util/xbee.h"

void XBeeUtil::address_to_bytes(uint64_t src, uint8_t *dest) {
	for (unsigned int i = 0; i < 8; i++) {
		dest[7 - i] = src & 0xFF;
		src >>= 8;
	}
}

uint64_t XBeeUtil::address_from_bytes(const uint8_t *src) {
	uint64_t ret = 0;
	for (unsigned int i = 0; i < 8; i++) {
		ret = (ret << 8) | src[i];
	}
	return ret;
}

