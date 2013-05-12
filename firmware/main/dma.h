#ifndef DMA_H
#define DMA_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief The possible DMA read channels.
 */
typedef enum {
	DMA_READ_CHANNEL_0,
	DMA_READ_CHANNEL_COUNT,
} dma_read_channel_t;

/**
 * \brief The possible DMA write channels.
 */
typedef enum {
	DMA_WRITE_CHANNEL_MRF = DMA_READ_CHANNEL_COUNT,
	DMA_WRITE_CHANNEL_2,
} dma_write_channel_t;

/**
 * \brief This function is used internally and should not be called by applications.
 */
void dma_start_impl(uint8_t channel, uint16_t data, uint8_t length);

/**
 * \brief Starts a DMA read channel.
 */
static inline void dma_read_start(dma_read_channel_t channel, const volatile void *data, uint8_t length) {
	dma_start_impl(channel, (uint16_t) data, length);
}

/**
 * \brief Starts a DMA write channel.
 */
static inline void dma_write_start(dma_write_channel_t channel, volatile void *data, uint8_t length) {
	dma_start_impl(channel, (uint16_t) data, length);
}

/**
 * \brief Starts a new transfer on a channel that begins where the previous transfer on the same channel ended.
 */
void dma_continue(uint8_t channel, uint8_t length);

/**
 * \brief Checks whether a DMA channel is currently running.
 */
bool dma_running(uint8_t channel);

#endif

