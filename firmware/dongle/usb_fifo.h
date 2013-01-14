#ifndef USB_FIFO_H
#define USB_FIFO_H

#include "stddef.h"

/**
 * \file
 * 
 * \brief Provides an allocator for, and the ability to flush, USB FIFOs.
 */



/**
 * \brief Sets all transmit FIFOs to their minimum sizes, ready to be set to other sizes later.
 * The FIFOs are also flushed.
 *
 * \pre All nonzero endpoints must be NAKing.
 */
void usb_fifo_reset(void);

/**
 * \brief Gets the starting offset of a transmit FIFO.
 *
 * \param[in] fifo the endpoint whose FIFO offset should be returned, from 1 to 3.
 *
 * \return the offset, in words.
 */
size_t usb_fifo_get_offset(unsigned int fifo);

/**
 * \brief Gets the size of a transmit FIFO.
 *
 * \param[in] fifo the endpoint whose FIFO size should be returned, from 1 to 3.
 *
 * \return the size, in words.
 */
size_t usb_fifo_get_size(unsigned int fifo);

/**
 * \brief Sets the size of a transmit FIFO.
 *
 * \pre The endpoint that uses the FIFO must be inactive.
 *
 * \param[in] fifo the endpoint whose FIFO size should be set, from 1 to 3.
 *
 * \param[in] size the size of the FIFO, in words.
 */
void usb_fifo_set_size(unsigned int fifo, size_t size);

/**
 * \brief Flushes a transmit FIFO.
 *
 * \pre The endpoint must be NAKing, due to either local or global NAK status.
 *
 * \param[in] fifo the endpoint whose FIFO should be flushed, from 1 to 3.
 */
void usb_fifo_flush(unsigned int fifo);

#endif

