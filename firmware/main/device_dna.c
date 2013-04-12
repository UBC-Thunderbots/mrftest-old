#include "device_dna.h"
#include "io.h"

uint64_t device_dna_read(void) {
	// Wait for initialization
	while (!(DEVICE_ID_STATUS & 0x01));

	// Read out bytes
	uint64_t id = 0;
	id |= DEVICE_ID6;
	id <<= 8;
	id |= DEVICE_ID5;
	id <<= 8;
	id |= DEVICE_ID4;
	id <<= 8;
	id |= DEVICE_ID3;
	id <<= 8;
	id |= DEVICE_ID2;
	id <<= 8;
	id |= DEVICE_ID1;
	id <<= 8;
	id |= DEVICE_ID0;
	return id;
}

