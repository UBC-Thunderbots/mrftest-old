#ifndef USB_H
#define USB_H

#include "stdbool.h"
#include "stdint.h"

/**
 * \file
 *
 * \brief Provides functionality for acting as a USB device
 */



/**
 * \name Global device information types
 *
 * These data types define global information about the device as a whole.
 *
 * @{
 */

/**
 * \brief A collection of information about the device
 */
typedef struct {
	/**
	 * \brief The number of words to allocate for the receive FIFO
	 */
	uint16_t rx_fifo_words;

	/**
	 * \brief The maximum packet size on endpoint 0 (must be one of 8, 16, 32, or 64)
	 */
	uint8_t ep0_max_packet;
} usb_device_info_t;

/**
 * @}
 */



/**
 * \name Device-wide functions
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
 * \param[in] device_info the device information structure defining the device.
 */
void usb_attach(const usb_device_info_t *device_info);

/**
 * \brief Detaches from the bus.
 *
 * \pre Callback context must not be executing.
 */
void usb_detach(void);

/**
 * @}
 */



/**
 * \name IN endpoint functions
 *
 * These functions handle operations related to one IN endpoint.
 *
 * @{
 */

/**
 * \brief Attaches a callback to handle IN endpoint interrupts
 *
 * \param[in] ep the endpoint number
 *
 * \param[in] cb the callback to register
 */
void usb_in_set_callback(uint8_t ep, void (*cb)(void));

/**
 * @}
 */

#endif

