#ifndef DMA_H
#define DMA_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief The possible DMA read channels.
 */
typedef enum {
	DMA_READ_CHANNEL_SD,
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
void dma_start_impl(uint8_t channel, uint16_t data, uint16_t length);

/**
 * \brief Starts a DMA read channel.
 *
 * \param[in] channel the read channel to start
 *
 * \param[in] data the data to send to the peripheral
 *
 * \param[in] the number of bytes to send
 */
static inline void dma_read_start(dma_read_channel_t channel, const volatile void *data, uint16_t length) {
	dma_start_impl(channel, (uint16_t) data, length);
}

/**
 * \brief Starts a DMA write channel.
 *
 * \param[in] channel the write channel to start
 *
 * \param[in] data a buffer in which to store received bytes
 *
 * \param[in] the number of bytes to receive
 */
static inline void dma_write_start(dma_write_channel_t channel, volatile void *data, uint16_t length) {
	dma_start_impl(channel, (uint16_t) data, length);
}

/**
 * \brief Checks whether a DMA channel is currently running.
 */
bool dma_running(uint8_t channel);

#endif

