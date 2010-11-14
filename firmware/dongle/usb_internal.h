#ifndef USB_INTERNAL_H
#define USB_INTERNAL_H

/**
 * \file
 *
 * \brief Definitions and declarations used only internally inside the USB subsystem.
 */

#include "usb_config.h"
#include <stdint.h>

/**
 * \brief The current device info structure.
 */
extern __code const usb_devinfo_t *usb_devinfo;

/**
 * \brief Initializes the low-level USB layer, placing the device into the POWERED state and awaiting a bus reset.
 */
void usb_lowlevel_init(void);

/**
 * \brief Deinitializes the low-level USB layer and disconnects the device from the bus.
 */
void usb_lowlevel_deinit(void);

/**
 * \brief Initializes the endpoint zero USB module.
 *
 * This function sets transaction callbacks for endpoint zero and enables the endpoint.
 */
void usb_ep0_init(void);

#endif

