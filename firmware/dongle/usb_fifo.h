#ifndef USB_FIFO_H
#define USB_FIFO_H

#include "stddef.h"

/**
 * \file
 * 
 * \brief Provides an allocator for, and the ability to flush, USB FIFOs.
 */



/**
 * \brief Initializes the FIFO manager and sets up the receive and endpoint-zero transmit FIFOs.
 * All other FIFOs are set to their minimum sizes.
 *
 * All FIFOs are flushed.
 *
 * This function should be called as a result of USB reset signalling.
 *
 * \param rx_size the size, in bytes, of the receive FIFO (which is shared between all endpoint zero and all nonzero OUT endpoints), which must be a multiple of 4 and at least 64
 *
 * \param tx0_size the size, in bytes, of the transmit FIFO for IN endpoint zero, which must be a multiple of 4 and at least 64
 */
void usb_fifo_init(size_t rx_size, size_t tx0_size);

/**
 * \brief Enables a transmit FIFO.
 *
 * Enabling a transmit FIFO allocates space for it and flushes it.
 * The FIFO must be enabled before an endpoint can use it.
 * Before enabling a FIFO, numerically lower FIFOs must be enabled.
 *
 * \param fifo the index of the FIFO to enable, from 1 to 3
 *
 * \param size the size, in bytes, of the FIFO, which must be a multiple of 4 and at least 64
 */
void usb_fifo_enable(unsigned int fifo, size_t size);

/**
 * \brief Disables a transmit FIFO.
 *
 * Disabling a transmit FIFO deallocates all space used by the FIFO.
 * Before disabling a FIFO, numerically higher FIFOs must be disabled.
 *
 * \param fifo the index of the FIFO to disable, from 1 to 3
 */
void usb_fifo_disable(unsigned int fifo);

/**
 * \brief Flushes a transmit FIFO.
 *
 * \pre The endpoint must be inactive.
 *
 * \param fifo the endpoint whose FIFO should be flushed, from 0 to 3
 */
void usb_fifo_flush(unsigned int fifo);

/**
 * \brief Returns the size of a FIFO.
 *
 * \param fifo the endpoint whose FIFO should be examined, from 0 to 3
 *
 * \return the size of the FIFO, in bytes
 */
size_t usb_fifo_get_size(unsigned int fifo);

#endif

