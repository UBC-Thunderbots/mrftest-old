#ifndef USB_BI_IN_H
#define USB_BI_IN_H

/**
 * \file
 *
 * \brief Provides handling of bulk and interrupt IN endpoints.
 */

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

/**
 * \brief The possible types of endpoints handled by this module.
 */
typedef enum {
	/**
	 * \brief A bulk endpoint.
	 */
	USB_BI_IN_EP_TYPE_BULK = 2,

	/**
	 * \brief An interrupt endpoint.
	 */
	USB_BI_IN_EP_TYPE_INTERRUPT = 3,
} usb_bi_in_ep_type_t;

/**
 * \brief The possible states an endpoint handled by this module can be in.
 */
typedef enum {
	/**
	 * \brief The endpoint is not initialized.
	 */
	USB_BI_IN_STATE_UNINITIALIZED,

	/**
	 * \brief The endpoint is initialized but no transfer is running.
	 */
	USB_BI_IN_STATE_IDLE,

	/**
	 * \brief The endpoint is currently running a transfer.
	 */
	USB_BI_IN_STATE_ACTIVE,

	/**
	 * \brief The endpoint is currently halted.
	 */
	USB_BI_IN_STATE_HALTED,
} usb_bi_in_state_t;

/**
 * \brief Returns the state of an endpoint.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \return the current endpoint state
 */
usb_bi_in_state_t usb_bi_in_get_state(unsigned int ep);

/**
 * \brief Initializes an IN endpoint for bulk or interrupt operation.
 *
 * This is typically called when entering a configuration or switching interface alternate settings.
 * The application is expected to initialize the relevant transmit FIFO to the appropriate size (at least large enough to hold two maximum-sized packets) and flush it before calling this function.
 * The FIFO should not be moved or resized until the endpoint is deinitialized, though if necessary it may be flushed as long as no transfer is running.
 *
 * \warning
 * The chip manual states that no more than eight packets may be in the transmit FIFO at a time, regardless of the sizes of the packets, the FIFO, or the running transfer.
 * This module works around this limitation if necessary; however, the workaround may negatively impact performance for some traffic patterns.
 * Thus, if an endpoint will be used to issue transfers comprising large numbers of small packets, the FIFO size should be at most 8× the maximum packet size.
 * This avoids the need for the transfer to be split up into many 8-packet physical transfers for flow control reasons.
 * There is no performance impact for having an overly large FIFO if all transfers will be smaller than 8 packets anyway.
 *
 * \pre The endpoint is in \ref USB_BI_IN_STATE_UNINITIALIZED.
 *
 * \post The endpoint is in \ref USB_BI_IN_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param max_packet the maximum packet size for this endpoint, in bytes
 *
 * \param type the type of endpoint
 */
void usb_bi_in_init(unsigned int ep, size_t max_packet, usb_bi_in_ep_type_t type);

/**
 * \brief Disables an IN endpoint.
 *
 * This is typically called when exiting a configuration or switching interface alternate settings.
 * If the endpoint is active, the transfer is aborted.
 * If the endpoint is halted, the halt is cleared.
 *
 * \post The endpoint is in \ref USB_BI_IN_STATE_UNINITIALIZED.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_in_deinit(unsigned int ep);

/**
 * \brief Halts an IN endpoint.
 *
 * \pre The endpoint is in \ref USB_BI_IN_STATE_IDLE.
 *
 * \post The endpoint is in \ref USB_BI_IN_STATE_HALTED.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_in_halt(unsigned int ep);

/**
 * \brief Takes an IN endpoint out of halt status.
 *
 * \pre The endpoint is in \ref USB_BI_IN_STATE_HALTED.
 *
 * \post The endpoint is in \ref USB_BI_IN_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_in_clear_halt(unsigned int ep);

/**
 * \brief Resets an IN endpoint’s data PID.
 *
 * This is typically done in response to certain control transfers targetting the endpoint.
 * The application does not need to call this function after initializing the endpoint; \ref usb_bi_in_init automatically sets the PID to DATA0.
 *
 * \pre The endpoint is in \ref USB_BI_IN_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_in_reset_pid(unsigned int ep);

/**
 * \brief Starts a transfer on an IN endpoint.
 *
 * A transfer must be started \em before data can be pushed into the transmit FIFO.
 * The application should probably push data into the transmit FIFO immediately after this function returns.
 * However, if the application registered an \p on_space callback, it may alternatively wait until that callback is invoked before pushing data.
 *
 * The application can send a zero-length transfer consisting of a sole zero-length packet by setting \p length to zero and \p max_length to any nonzero value.
 *
 * \pre The endpoint is in \ref USB_BI_IN_STATE_IDLE.
 *
 * \post The endpoint is in \ref USB_BI_IN_STATE_ACTIVE.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param length the number of bytes to transfer
 *
 * \param max_length the maximum transfer size the host is expecting; if \p length is equal to \p max_length, the terminating zero-length packet will be omitted
 *
 * \param on_complete a callback to invoke when the transfer is complete; may be null if not needed; within this callback, no transfer is running so another transfer can be started
 *
 * \param on_space a callback to invoke when space is available in the transmit FIFO to hold another packet (see \ref usb_bi_in_push for details); may be null if not needed; if provided, this callback \em must push data into the transmit FIFO before returning
 */
void usb_bi_in_start_transfer(unsigned int ep, size_t length, size_t max_length, void (*on_complete)(void), void (*on_space)(void));

/**
 * \brief Aborts a running transfer on an IN endpoint.
 *
 * This function does not flush the FIFO.
 * The application should do that once this function returns.
 *
 * \pre The endpoint is in \ref USB_BI_IN_STATE_ACTIVE.
 *
 * \post The endpoint is in \ref USB_BI_IN_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_in_abort_transfer(unsigned int ep);

/**
 * \brief Pushes a block of data into the transmit FIFO.
 *
 * This function returns the number of bytes actually consumed.
 * The application may or may not actually need to check this return value, because there are two basic ways to handle pushing data into a transfer.
 *
 * One way to handle pushing data works well for transfers that are always fairly small.
 * If transfers are always of a limited and fairly small size, the application can allocate a transmit FIFO large enough to hold a complete transfer.
 * The application can then push the entire transfer’s worth of data immediately after starting the transfer, ignoring return values, and not provide an \p on_space callback to \ref usb_bi_in_start_transfer.
 *
 * Alternatively, if transfers may be large, the transmit FIFO cannot be made large enough to hold a whole transfer.
 * In this case, the application must check the return value from this function to detect when the transmit FIFO is full.
 * Once a full FIFO is detected, the application can do other work.
 * When space is available in the FIFO for more data, the \p on_space callback passed to \ref usb_bi_in_start_transfer is invoked.
 * In that callback, the application pushes more data into the transmit FIFO.
 * Note that in this case, the application need not actually push \em any data immediately after starting the transfer; it can instead simply wait for \p on_space to be called before pushing the initial data.
 *
 * \pre The endpoint must be in \ref USB_BI_IN_STATE_ACTIVE.
 *
 * \pre The amount of data provided must not be more than needed to finish the transfer.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param data the data to push
 *
 * \param length the number of bytes to push
 *
 * \return the number of bytes actually consumed
 */
size_t usb_bi_in_push(unsigned int ep, const void *data, size_t length);

#endif

