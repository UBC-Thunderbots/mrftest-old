#ifndef USB_BI_OUT_H
#define USB_BI_OUT_H

/**
 * \file
 *
 * \brief Provides handling of bulk and interrupt OUT endpoints.
 */

#include "stddef.h"

/**
 * \brief The possible types of endpoints handled by this module.
 */
typedef enum {
	/**
	 * \brief Indicates a bulk endpoint.
	 */
	USB_BI_OUT_EP_TYPE_BULK = 2,

	/**
	 * \brief Indicates an interrupt endpoint.
	 */
	USB_BI_OUT_EP_TYPE_INTERRUPT = 3,
} usb_bi_out_ep_type_t;

/**
 * \brief The possible states an endpoint handled by this module can be in.
 */
typedef enum {
	/**
	 * \brief Indicates the endpoint is not initialized.
	 */
	USB_BI_OUT_STATE_UNINITIALIZED,

	/**
	 * \brief Indicates the endpoint is initialized but no transfer is running.
	 */
	USB_BI_OUT_STATE_IDLE,

	/**
	 * \brief Indicates the endpoint is currently running a transfer.
	 */
	USB_BI_OUT_STATE_ACTIVE,

	/**
	 * \brief Indicates that the endpoint is currently halted.
	 */
	USB_BI_OUT_STATE_HALTED,
} usb_bi_out_state_t;

/**
 * \brief Returns the state of an endpoint.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 *
 * \return the current endpoint state
 */
usb_bi_out_state_t usb_bi_out_get_state(unsigned int ep);

/**
 * \brief Initializes an OUT endpoint for bulk or interrupt operation.
 *
 * This is typically called when entering a configuration or switching interface alternate settings.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_UNINITIALIZED.
 *
 * \post The endpoint is in USB_BI_OUT_STATE_IDLE.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 *
 * \param[in] max_packet the maximum packet size for this endpoint, in bytes
 *
 * \param[in] type the type of endpoint
 */
void usb_bi_out_init(unsigned int ep, size_t max_packet, usb_bi_out_ep_type_t type);

/**
 * \brief Disables an OUT endpoint.
 *
 * This is typically called when exiting a configuration or switching interface alternate settings.
 * If the endpoint is active, the transfer is aborted.
 * If the endpoint is halted, the halt is cleared.
 *
 * \post The endpoint is in USB_BI_OUT_STATE_UNINITIALIZED.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 */
void usb_bi_out_deinit(unsigned int ep);

/**
 * \brief Halts an OUT endpoint.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_IDLE.
 *
 * \post The endpoint is in USB_BI_OUT_STATE_HALTED.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 */
void usb_bi_out_halt(unsigned int ep);

/**
 * \brief Takes an OUT endpoint out of halt status.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_HALTED.
 *
 * \post The endpoint is in USB_BI_OUT_STATE_IDLE.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 */
void usb_bi_out_clear_halt(unsigned int ep);

/**
 * \brief Resets an OUT endpoint’s data PID.
 *
 * This is typically done in response to certain control transfers targetting the endpoint.
 * The application does not need to call this function after initializing the endpoint; \ref usb_bi_out_init automatically sets the PID to DATA0.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_IDLE.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 *
 * \param[in] pid the PID to set as expected for the next packet, either 0 or 1
 */
void usb_bi_out_reset_pid(unsigned int ep, unsigned int pid);

/**
 * \brief Starts a transfer on an OUT endpoint.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_IDLE.
 *
 * \post The endpoint is in USB_BI_OUT_STATE_ACTIVE.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 *
 * \param[in] max_length the maximum transfer size the device is expecting; if the number of bytes the host sends is equal to \p max_length, the transfer will complete without the host sending a zero-length packet
 *
 * \param[in] on_complete a callback to invoke when the transfer is complete; may be null if not needed; within this callback, no transfer is running so another transfer can be started
 *
 * \param[in] on_packet a callback to invoke when a packet is available in the receive FIFO; this callback must read the packet data; the parameter is the size of the received packet (but be aware that this size could extend beyond the maximum transfer length, if the host is noncompliant)
 */
void usb_bi_out_start_transfer(unsigned int ep, size_t max_length, void (*on_complete)(void), void (*on_packet)(size_t));

/**
 * \brief Aborts a running transfer on an OUT endpoint.
 *
 * \pre Global NAK is effective.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_ACTIVE.
 *
 * \post The endpoint is in USB_BI_OUT_STATE_IDLE.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 */
void usb_bi_out_abort_transfer(unsigned int ep);

/**
 * \brief Reads a block of data from a received packet.
 *
 * The application must call this function when the \c on_packet callback is invoked.
 * The application is responsible for not requesting more bytes than the size of the packet.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_ACTIVE.
 *
 * \pre The \c on_packet callback registered in \ref usb_bi_out_start_transfer is executing.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 *
 * \param[in] dst the location to store the data at
 *
 * \param[in] length the number of bytes to read from the packet and write to \p dst
 */
void usb_bi_out_read(unsigned int ep, void *dst, size_t length);

/**
 * \brief Reads a block of data from a received packet and discards it.
 *
 * The application must call this function when the \c on_packet callback is invoked.
 * The application is responsible for not requesting more bytes than the size of the packet.
 *
 * \pre The endpoint is in USB_BI_OUT_STATE_ACTIVE.
 *
 * \pre The \c on_packet callback registered in \ref usb_bi_out_start_transfer is executing.
 *
 * \param[in] ep the endpoint number, from 1 to 3
 *
 * \param[in] length the number of bytes to read from the packet
 */
void usb_bi_out_discard(unsigned int ep, size_t length);

#endif

