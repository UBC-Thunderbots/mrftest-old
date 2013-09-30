#ifndef USB_CONFIGS_H
#define USB_CONFIGS_H

/**
 * \file
 *
 * \brief Provides a layer that handles switching between configurations.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/**
 * \brief A set of callbacks to handle activities related to a single configuration.
 */
typedef struct {
	/**
	 * \brief The configuration value of the configuration.
	 */
	uint8_t configuration;

	/**
	 * \brief Checks whether it’s acceptable to enter this configuration at this time.
	 *
	 * This callback is optional; if not provided, requests to enter the configuration always succeed.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 *
	 * \return \c true if the request is acceptable, or \c false if not
	 */
	bool (*can_enter)(void);

	/**
	 * \brief Enters the configuration.
	 *
	 * This callback is optional.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 */
	void (*on_enter)(void);

	/**
	 * \brief Exits the configuration.
	 *
	 * This callback is optional.
	 *
	 * \pre Callback context is executing.
	 *
	 * \pre Global NAK is effective in both directions.
	 */
	void (*on_exit)(void);
} usb_configs_config_t;

/**
 * \brief Initializes the configuration management module.
 *
 * This should be invoked as a result of setting a nonzero device address.
 * Initializing the module when it is already initialized re-initializes the current configuration to zero and replaces the available configuration set; the on-exit handler for the current configuration is \em not invoked.
 *
 * \param configs a pointer to an array of pointers to configuration structures, one per configuration, ending with a null pointer
 */
void usb_configs_init(const usb_configs_config_t * const *configs);

/**
 * \brief Deinitializes the configuration management module.
 *
 * This should be invoked as a result of bus detachment or setting a zero device address.
 * Deinitializing the module when it is not initialized is harmless.
 */
void usb_configs_deinit(void);

/**
 * \brief Returns the configuration number for the currently active configuration.
 *
 * \return the current configuration
 */
uint8_t usb_configs_get_current(void);

#endif

