#ifndef STM32LIB_USB_BI_OUT_H
#define STM32LIB_USB_BI_OUT_H

/**
 * \file
 *
 * \brief Provides handling of bulk and interrupt OUT endpoints.
 */

#include <stdbool.h>
#include <stddef.h>

/**
 * \brief The possible types of endpoints handled by this module.
 */
typedef enum {
	/**
	 * \brief A bulk endpoint.
	 */
	USB_BI_OUT_EP_TYPE_BULK = 2,

	/**
	 * \brief An interrupt endpoint.
	 */
	USB_BI_OUT_EP_TYPE_INTERRUPT = 3,
} usb_bi_out_ep_type_t;

/**
 * \brief The possible states an endpoint handled by this module can be in.
 */
typedef enum {
	/**
	 * \brief The endpoint is not initialized.
	 */
	USB_BI_OUT_STATE_UNINITIALIZED,

	/**
	 * \brief The endpoint is initialized but no transfer is running.
	 */
	USB_BI_OUT_STATE_IDLE,

	/**
	 * \brief The endpoint is currently running a transfer.
	 */
	USB_BI_OUT_STATE_ACTIVE,

	/**
	 * \brief The endpoint is currently halted.
	 */
	USB_BI_OUT_STATE_HALTED,
} usb_bi_out_state_t;

/**
 * \brief The type of a callback invoked by the standard halt handling layer just after an endpoint enters halt.
 *
 * This callback is not invoked if the application caused the halt by calling \ref usb_bi_out_halt.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_HALTED.
 *
 * \param ep the endpoint number, from 1 to 3
 */
typedef void (*usb_bi_out_enter_halt_cb_t)(unsigned int ep);

/**
 * \brief The type of a callback invoked by the standard halt handling layer just before an endpoint exits halt.
 *
 * This callback is not invoked if the host requests to exit halt when the endpoint is not halted.
 * This callback is also not invoked if the application caused the exit by calling \ref usb_bi_out_clear_halt.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_HALTED.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \return \c true to allow the endpoint to exit halt, or \c false to disallow it
 */
typedef bool (*usb_bi_out_pre_exit_halt_cb_t)(unsigned int ep);

/**
 * \brief The type of a callback invoked by the standard halt handling layer just after an endpoint exits halt.
 *
 * This callback is also invoked if the host requests to exit halt when the endpoint is not halted.
 * This callback is also not invoked if the application caused the exit by calling \ref usb_bi_out_clear_halt.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 */
typedef void (*usb_bi_out_post_exit_halt_cb_t)(unsigned int ep);

/**
 * \brief Returns the state of an endpoint.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \return the current endpoint state
 */
usb_bi_out_state_t usb_bi_out_get_state(unsigned int ep);

/**
 * \brief Initializes an OUT endpoint for bulk or interrupt operation.
 *
 * This is typically called when entering a configuration or switching interface alternate settings.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_UNINITIALIZED.
 *
 * \post The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param max_packet the maximum packet size for this endpoint, in bytes
 *
 * \param type the type of endpoint
 */
void usb_bi_out_init(unsigned int ep, size_t max_packet, usb_bi_out_ep_type_t type);

/**
 * \brief Disables an OUT endpoint.
 *
 * This is typically called when exiting a configuration or switching interface alternate settings.
 * If the endpoint is active, the transfer is aborted.
 * If the endpoint is halted, the halt is cleared.
 * If standard halt handling was enabled for the endpoint, it is disabled.
 *
 * \post The endpoint is in \ref USB_BI_OUT_STATE_UNINITIALIZED.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_out_deinit(unsigned int ep);

/**
 * \brief Configures standard endoint halt handling for an OUT endpoint.
 *
 * The application may handle endpoint halt requests itself, or may call this function for standard handling.
 * This handles SET FEATURE(HALT), CLEAR FEATURE(HALT), and GET STATUS(ENDPOINT) requests.
 * Standard handling notifies the application on requests to enter halt, and also allows the application to veto clearing of halt.
 * This function may be called when standard halt handling is already set up for a particular endpoint; doing this modifies the callbacks.
 *
 * Any of the callbacks may be null, in which case they are not invoked (and the pre-exit callback is taken to accept the request).
 *
 * \pre The endpoint is not in \ref USB_BI_OUT_STATE_UNINITIALIZED.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param enter_halt_cb a callback invoked after entering halt status
 *
 * \param pre_exit_halt_cb a callback invoked just before exiting halt status, which allows vetoing
 *
 * \param post_exit_halt_cb a callback invoked just after exiting halt status
 */
void usb_bi_out_set_std_halt(unsigned int ep, usb_bi_out_enter_halt_cb_t enter_halt_cb, usb_bi_out_pre_exit_halt_cb_t pre_exit_halt_cb, usb_bi_out_post_exit_halt_cb_t post_exit_halt_cb);

/**
 * \brief Halts an OUT endpoint.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \post The endpoint is in \ref USB_BI_OUT_STATE_HALTED.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_out_halt(unsigned int ep);

/**
 * \brief Takes an OUT endpoint out of halt status.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_HALTED.
 *
 * \post The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_out_clear_halt(unsigned int ep);

/**
 * \brief Resets an OUT endpoint’s data PID.
 *
 * This is typically done in response to certain control transfers targetting the endpoint.
 * The application does not need to call this function after initializing the endpoint; \ref usb_bi_out_init automatically sets the PID to DATA0.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param pid the PID to set as expected for the next packet, either 0 or 1
 */
void usb_bi_out_reset_pid(unsigned int ep, unsigned int pid);

/**
 * \brief Starts a transfer on an OUT endpoint.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \post The endpoint is in \ref USB_BI_OUT_STATE_ACTIVE.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param max_length the maximum transfer size the device is expecting; if the number of bytes the host sends is equal to \p max_length, the transfer will complete without the host sending a zero-length packet
 *
 * \param on_complete a callback to invoke when the transfer is complete; may be null if not needed; within this callback, no transfer is running so another transfer can be started
 *
 * \param on_packet a callback to invoke when a packet is available in the receive FIFO; this callback must read the packet data; the parameter is the size of the received packet (but be aware that this size could extend beyond the maximum transfer length, if the host is noncompliant); any data not read by the callback is automatically discarded after it returns
 */
void usb_bi_out_start_transfer(unsigned int ep, size_t max_length, void (*on_complete)(void), void (*on_packet)(size_t));

/**
 * \brief Aborts a running transfer on an OUT endpoint.
 *
 * After a transfer is aborted, no further calls to the registered callbacks will be made (at least not until a new transfer is started for the endpoint).
 *
 * \pre Global NAK is effective.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_ACTIVE.
 *
 * \post The endpoint is in \ref USB_BI_OUT_STATE_IDLE.
 *
 * \param ep the endpoint number, from 1 to 3
 */
void usb_bi_out_abort_transfer(unsigned int ep);

/**
 * \brief Reads a block of data from a received packet.
 *
 * The application must call this function or \ref usb_bi_out_discard when the \c on_packet callback registered with \ref usb_bi_out_start_transfer is invoked.
 * The application is responsible for not requesting more bytes than the size of the packet.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_ACTIVE.
 *
 * \pre The \c on_packet callback registered in \ref usb_bi_out_start_transfer is executing.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param dst the location to store the data at
 *
 * \param length the number of bytes to read from the packet and write to \p dst
 */
void usb_bi_out_read(unsigned int ep, void *dst, size_t length);

/**
 * \brief Reads a block of data from a received packet and discards it.
 *
 * The application must call this function or \ref usb_bi_out_read when the \c on_packet callback registered with \ref usb_bi_out_start_transfer is invoked.
 * The application is responsible for not requesting more bytes than the size of the packet.
 *
 * \pre The endpoint is in \ref USB_BI_OUT_STATE_ACTIVE.
 *
 * \pre The \c on_packet callback registered in \ref usb_bi_out_start_transfer is executing.
 *
 * \param ep the endpoint number, from 1 to 3
 *
 * \param length the number of bytes to read from the packet
 */
void usb_bi_out_discard(unsigned int ep, size_t length);

#endif

