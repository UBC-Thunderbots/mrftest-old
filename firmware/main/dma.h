#ifndef DMA_H
#define DMA_H

#include <stdbool.h>
#include <stddef.h>

/**
 * \brief A handle to a DMA-capable, suitably aligned memory block.
 */
typedef struct dma_memory_handle *dma_memory_handle_t;

void dma_init(void);
bool dma_check(const void *pointer, size_t length);
dma_memory_handle_t dma_alloc(size_t length);
void dma_free(dma_memory_handle_t handle);
void *dma_get_buffer(dma_memory_handle_t handle);

#endif

