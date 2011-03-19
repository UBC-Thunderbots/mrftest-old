#ifndef USB_CONFIG_H
#define USB_CONFIG_H

/**
 * \file
 *
 * \brief Holds editable values to configure the USB subsystem.
 */

/**
 * \brief Set to 1 to use an external transceiver, or 0 to use the internal transceiver.
 */
#define USB_CONFIG_EXTERNAL_TRANSCEIVER 0

/**
 * \brief Set to 1 to operate in full-speed mode, or 0 to operate in low-speed mode.
 */
#define USB_CONFIG_FULL_SPEED 1

/**
 * \brief Set to 1 to enable the on-chip pullup resistors, or 0 to disable them.
 */
#define USB_CONFIG_ONBOARD_PULLUPS 1

/**
 * \brief Set to 1 to set USB interrupts as high priority, or 0 to set them as low priority.
 */
#define USB_CONFIG_INTERRUPTS_HIGH_PRIORITY 0

/**
 * \brief Set to The largest endpoint index (from 0 to 15) used by the device.
 */
#define USB_CONFIG_MAX_ENDPOINT 6

/**
 * \brief The number of scatter-gather descriptors to allocate for endpoint zero transfers.
 */
#define USB_CONFIG_NUM_EP0_IOVECS 2

/**
 * \brief Set to 1 if the device is self-powered, or 0 if bus-powered.
 */
#define USB_CONFIG_SELF_POWERED 0

/**
 * \brief Set to 1 if a start-of-frame callback is needed, or 0 if not.
 */
#define USB_CONFIG_SOF_CALLBACK 0

/**
 * \brief Set to 1 if string descriptors are used, or 0 if not.
 */
#define USB_CONFIG_STRING_DESCRIPTORS 1

/**
 * \brief Set to 1 if idle mode is implemented, or 0 if not.
 */
#define USB_CONFIG_IDLE 0

/**
 * \brief Set to 1 if endpoint halt is implemented, or 0 if not.
 */
#define USB_CONFIG_HALT 0

#endif

