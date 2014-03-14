#ifndef STM32LIB_USB_ALTSETTINGS_H
#define STM32LIB_USB_ALTSETTINGS_H

/**
 * \file
 *
 * \brief Provides a layer that handles switching an interface between alternate settings.
 *
 * When this module is enabled for an interface, it handles the GET INTERFACE and SET INTERFACE control requests addressed to that interface.
 * It does not handle GET STATUS control requests to the interface.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/**
 * \brief The maximum number of interfaces supported by this module.
 */
#define USB_ALTSETTINGS_MAX_INTERFACES 8

/**
 * \brief A set of callbacks to handle activities related to a single alternate setting.
 */
typedef struct {
	/**
	 * \brief Checks whether it’s acceptable to enter this alternate setting at this time.
	 *
	 * This callback is optional; if not provided, requests to enter the alternate setting always succeed.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 *
	 * \return \c true if the request is acceptable, or \c false if not
	 */
	bool (*can_enter)(void);

	/**
	 * \brief Enters the alternate setting.
	 *
	 * This callback is optional.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 */
	void (*on_enter)(void);

	/**
	 * \brief Exits the alternate setting.
	 *
	 * This callback is optional.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 */
	void (*on_exit)(void);
} usb_altsettings_altsetting_t;

/**
 * \brief Initializes the alternate setting management module for a particular interface.
 *
 * This should be invoked as a result of entering a configuration.
 * The on-enter handler for the zeroth alternate setting (if present) is invoked, but the can-enter handler (if present) is not.
 * Invoking this function for an interface that already has alternate settings configured changes the available set of alternate settings, invoking the exit handler for the old current alternate setting and the entry handler for the new alternate setting zero, as well as resetting the current alternate setting to zero.
 *
 * \param interface the interface number to set up, in the range 0 through \ref USB_ALTSETTINGS_MAX_INTERFACES − 1
 *
 * \param altsettings a pointer to an array of pointers to alternate setting structures, one per alternate setting, ending with a null pointer
 */
void usb_altsettings_init(unsigned int interface, const usb_altsettings_altsetting_t * const *altsettings);

/**
 * \brief Deinitializes the alternate setting management module for a particular interface.
 *
 * This should be invoked as a result of exiting a configuration.
 * The on-exit handler for the current alternate setting (if present) is invoked.
 * Invoking this function for an interface that does not have alternate settings configured silently does nothing.
 *
 * \param interface the interface number to set up, in the range 0 through \ref USB_ALTSETTINGS_MAX_INTERFACES − 1
 */
void usb_altsettings_deinit(unsigned int interface);

/**
 * \brief Returns the index of the currently active alternate setting for an interface.
 *
 * \param interface the interface number to set up, in the range 0 through \ref USB_ALTSETTINGS_MAX_INTERFACES − 1
 *
 * \return the current alternate setting
 */
uint8_t usb_altsettings_get_current(unsigned int interface);

#endif

