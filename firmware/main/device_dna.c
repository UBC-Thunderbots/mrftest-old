#include "device_dna.h"
#include "io.h"

uint64_t device_dna_read(void) {
	while (!IO_SYSCTL.csr.device_dna_ready);
	return (((uint64_t) IO_SYSCTL.device_dna_high) << 32) | IO_SYSCTL.device_dna_low;
}

