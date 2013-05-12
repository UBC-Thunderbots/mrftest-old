#include "dma.h"
#include "io.h"

void dma_start_impl(uint8_t channel, uint16_t data, uint8_t length) {
	data -= 96; // Adjust pointer to be an offset into data memory, which is 96 bytes above address zero, after the registers and I/O ports.
	DMA_CHANNEL = channel;
	DMA_PTRL = data;
	DMA_PTRH = data >> 8;
	DMA_COUNT = length;
	DMA_CTL |= 1;
}

void dma_continue(uint8_t channel, uint8_t length) {
	DMA_CHANNEL = channel;
	DMA_COUNT = length;
	DMA_CTL |= 1;
}

bool dma_running(uint8_t channel) {
	DMA_CHANNEL = channel;
	return !!(DMA_CTL & 1);
}

