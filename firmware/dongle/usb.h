#ifndef USB_H
#define USB_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

/**
 * \file
 *
 * \brief Provides functionality for acting as a USB device.
 */



/**
 * \name Global device information types
 *
 * These data types define global information about the device as a whole.
 *
 * @{
 */

/**
 * \brief A collection of information about the device.
 */
typedef struct {
	/**
	 * \brief The number of words to allocate for the receive FIFO.
	 */
	uint16_t rx_fifo_words;

	/**
	 * \brief The maximum packet size on endpoint 0 (must be one of 8, 16, 32, or 64).
	 */
	uint8_t ep0_max_packet;

	/**
	 * \brief A callback invoked when a USB reset occurs.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 */
	void (*on_reset)(void);
} usb_device_info_t;

/**
 * @}
 */



/**
 * \name Device-wide functions.
 *
 * These functions handle bus attachment and detachment, entering the stack to do pending work, and other device-wide functionality.
 *
 * @{
 */

/**
 * \brief Handles USB activity.
 *
 * This function \em must be called quickly if a USB interrupt occurs.
 *
 * This function \em may be harmlessly called when a USB interrupt does not occur.
 *
 * All USB callbacks will be invoked as a consequence of, and in the same context (e.g. interrupt level) as, this function.
 */
void usb_process(void);

/**
 * \brief Initializes the device and attaches to the bus.
 *
 * While this function will enable all interrupt-handling machinery inside the USB core itself, it will not configure the NVIC.
 * The application must enable the interrupt in the NVIC if it wishes to use interrupts to handle USB activity.
 *
 * \pre A set of device callbacks must already be set.
 *
 * \pre The device must not already be attached (corollary: this function cannot be invoked from a callback).
 *
 * \param device_info the device information structure defining the device.
 */
void usb_attach(const usb_device_info_t *device_info);

/**
 * \brief Detaches from the bus.
 *
 * The detach operation does not complete immediately.
 * The function returns while the detach operation may still be taking place.
 *
 * \pre Callback context must not be executing.
 */
void usb_detach(void);

/**
 * @}
 */



/**
 * \name Endpoint callback management.
 *
 * These functions handle operations related to one IN endpoint.
 *
 * @{
 */

/**
 * \brief The type of a callback that handles IN endpoint interrupts.
 *
 * \param ep the endpoint number
 */
typedef void (*usb_in_callback_t)(unsigned int ep);

/**
 * \brief Attaches a callback to handle IN endpoint interrupts.
 *
 * \param ep the endpoint number
 *
 * \param cb the callback to register
 */
void usb_in_set_callback(unsigned int ep, usb_in_callback_t cb);

/**
 * \brief The type of a callback that handles OUT endpoint patterns.
 *
 * \param ep the endpoint number
 *
 * \param pattern the pattern to handle
 */
typedef void (*usb_out_callback_t)(unsigned int ep, uint32_t pattern);

/**
 * \brief Attaches a callback to handle OUT endpoint receive FIFO patterns.
 *
 * \param ep the endpoint number
 *
 * \param cb the callback to register, which accepts the FIFO pattern
 */
void usb_out_set_callback(unsigned int ep, usb_out_callback_t cb);

/**
 * @}
 */



/**
 * \name Global NAK handling.
 *
 * These functions and types allow an application to enter global NAK mode.
 *
 * @{
 */

/**
 * \brief A request for global NAK.
 *
 * The application must allocate a structure of this type in order to issue a global NAK request.
 * The application should not modify the elements of this structure.
 * The allocated structure should be initialized to \ref USB_GNAK_REQUEST_INIT.
 */
typedef struct usb_gnak_request {
	struct usb_gnak_request *next;
	bool queued;
	void (*cb)(void);
} usb_gnak_request_t;

/**
 * \brief The initialization value for a \ref usb_gnak_request_t.
 */
#define USB_GNAK_REQUEST_INIT { 0, false, 0 }

/**
 * \brief Requests for global NAK to occur.
 *
 * The USB stack will enter global NAK state as soon as possible and will then invoke the callback.
 * Global NAK state will end after the callback returns.
 *
 * It is safe to call this function with a request structure that has already been queued; in this case, nothing happens.
 * However, the application can change which callback function will be invoked by doing this.
 *
 * \param req the request structure identifying this request
 *
 * \param cb the callback to invoke when global NAK becomes effective, which may be null to effectively cancel a queued request
 */
void usb_set_global_nak(usb_gnak_request_t *req, void (*cb)(void));

/**
 * @}
 */

#endif

