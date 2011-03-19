#ifndef USB_H
#define USB_H

/**
 * \file
 *
 * \brief The USB subsystem.
 */

#include "signal.h"
#include "usb_config.h"
#include "usb_bd.h"
#include "usb_spec.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * \brief The information about a configuration.
 */
typedef struct {
	/**
	 * \brief The configuration descriptor and other descriptors following it.
	 */
	__code const usb_configuration_descriptor_t *configuration_descriptor;

	/**
	 * \brief The number of interfaces in the configuration.
	 */
	uint8_t num_interfaces;

	/**
	 * \brief The bitmask of IN endpoints that exist in this configuration (not including endpoint zero).
	 */
	uint16_t valid_in_endpoints;

	/**
	 * \brief The bitmask of OUT endpoints that exist in this configuration (not including endpoint zero).
	 */
	uint16_t valid_out_endpoints;

	/**
	 * \brief Invoked when the host activates the configuration.
	 */
	void (*on_enter)(void);

	/**
	 * \brief Invoked when the host deactivates the configuration.
	 */
	void (*on_exit)(void);
} usb_confinfo_t;

#if USB_CONFIG_STRING_DESCRIPTORS
/**
 * \brief A string table, which collects all the strings of a single language.
 */
typedef struct {
	/**
	 * \brief The language of the strings in this table.
	 */
	uint16_t language;

	/**
	 * \brief Pointers to the strings (which must themselves be NUL-terminated) in this table.
	 *
	 * The elements of this array are offset by 1, so that element 0 is string 1 and so on.
	 */
	__code const uint16_t *strings[];
} usb_string_table_t;

/**
 * \brief The string metatable, which collects all the languages' stringtables.
 */
typedef struct {
	/**
	 * \brief The number of stringtables.
	 */
	uint8_t length;

	/**
	 * \brief The number of strings in each table.
	 *
	 * All tables must contain the same number of strings, because string indices are all non-language-specific.
	 */
	uint8_t table_length;

	/**
	 * \brief Pointers to the stringtables.
	 */
	__code const usb_string_table_t *string_tables[];
} usb_string_metatable_t;
#endif

/**
 * \brief The root of a tree of structures defining the properties and operation of the USB subsystem for the device.
 */
typedef struct {
	/**
	 * \brief The device descriptor.
	 */
	__code const usb_device_descriptor_t *device_descriptor;

	/**
	 * \brief A handler to handle SETUP transactions on endpoint zero.
	 *
	 * Setting this pointer to a non-null value permits the application to handle custom endpoint-zero requests.
	 *
	 * If the custom handler handles the transaction, it should:
	 * \li Set usb_ep0_data and usb_ep0_data_length, if the SETUP packet indicated a nonzero length.
	 * \li Set usb_ep0_prestatus, if the application needs to obtain control between the data and status stages of the control transfer.
	 * \li Set usb_ep0_poststatus, if the application needs to obtain control after the status stage of the control transfer.
	 * \li Set usb_ep0_fail, if the application needs to obtain control in the event of transfer failure.
	 * \li Return \c true.
	 *
	 * \return \c true if the custom handler has handled the transaction, or \c false if not.
	 */
	BOOL (*custom_ep0_setup_handler)(void);

#if USB_CONFIG_STRING_DESCRIPTORS
	/**
	 * \brief A pointer to string descriptor zero.
	 */
	__code const void *string_descriptor_zero;

	/**
	 * \brief A pointer to the string metatable.
	 */
	__code const usb_string_metatable_t *string_metatable;
#endif

	/**
	 * \brief Details of the configurations supported by the device.
	 *
	 * The number of elements in the array is given by the num_configurations element of the device descriptor.
	 */
	__code const usb_confinfo_t *configurations[];
} usb_devinfo_t;

/**
 * \brief Initializes the USB subsystem.
 *
 * \param devinfo the usb_devinfo controlling the operation of the device.
 */
void usb_init(__code const usb_devinfo_t *devinfo);

/**
 * \brief Deinitializes the USB subsystem and disconnects the device from the bus.
 */
void usb_deinit(void);

/**
 * \brief Handles activity in the USB subsystem.
 *
 * This function is suitable for use as an interrupt handler,
 * but can also be called from a main loop.
 */
SIGHANDLER(usb_process);

#if USB_CONFIG_IDLE
/**
 * \brief Indicates whether the host has ordered the function to suspend.
 *
 * If the host orders a suspend, the USB subsystem automatically suspends the transceiver,
 * but it is up to the application to check this value regularly and put the CPU to sleep if appropriate.
 *
 * To avoid race conditions, it would be best to check this variable once, then, if it is \c true,
 * disable interrupts and check it again before sleeping.
 * If an interrupt occurs, the vector will not be called so the interrupt flag will remain set.
 * This set interrupt flag will wake the chip from sleep or prevent it from sleeping.
 * Interrupts can then be re-enabled to take the interrupt, and this variable retested.
 */
extern volatile BOOL usb_is_idle;
#endif

/**
 * \brief The index of the current configuration in the device info table configuration array,
 * or \c 0xFF if the device is unconfigured.
 *
 * The application must not modify this value.
 * It is modified by the USB subsystem as a result of control transfers from the host.
 *
 * This will generally not be equal to the configuration ID declared in a descriptor.
 */
extern volatile uint8_t usb_current_configuration;

#if USB_CONFIG_HALT
/**
 * \brief A bitmask indicating which IN endpoints are halted.
 *
 * The endpoint zero layer updates this variable.
 * The application may set bits in this variable in response to functional errors.
 */
extern uint16_t usb_halted_in_endpoints;

/**
 * \brief A bitmask indicating which OUT endpoints are halted.
 *
 * The endpoint zero layer updates this variable.
 * The application may set bits in this variable in response to functional errors.
 */
extern uint16_t usb_halted_out_endpoints;
#endif

/**
 * \brief The type of callbacks to handle activity on a single endpoint in a single direction.
 */
typedef struct {
	/**
	 * \brief Invoked when a transaction completes on the endpoint.
	 */
	void (*transaction)(void);

#if USB_CONFIG_HALT
	/**
	 * \brief Invoked when a control transfer orders a commanded functional stall on the endpoint.
	 */
	void (*commanded_stall)(void);

	/**
	 * \brief Invoked when a control transfer orders the endpoint to clear its halt status.
	 *
	 * \return \c true if the halt feature was successfully cleared, or \c false if a functional error condition still exists.
	 */
	BOOL (*clear_halt)(void);
#endif
} usb_ep_callbacks_t;

/**
 * \brief The type of callbacks to handle activity on a single endpoint number.
 */
typedef struct {
	/**
	 * \brief The callbacks for the outbound endpoint of this number.
	 */
	usb_ep_callbacks_t out;

	/**
	 * \brief The callbacks for the inbound endpoint of this number.
	 */
	usb_ep_callbacks_t in;
} usb_epn_callbacks_t;

/**
 * \brief The callbacks to handle activity on a single endpoint number.
 */
extern usb_epn_callbacks_t usb_ep_callbacks[USB_CONFIG_MAX_ENDPOINT + 1];

#if USB_CONFIG_SOF_CALLBACK
/**
 * \brief The callback to invoke at the start of each USB frame.
 */
extern void (*usb_sof_callback)(void);
#endif

/**
 * \brief The most recently received SETUP packet on endpoint zero.
 */
extern usb_setup_packet_t usb_ep0_setup_buffer;

/**
 * \brief The most recently received SETUP or OUT packet on endpoint zero.
 */
extern uint8_t usb_ep0_out_buffer[8];

/**
 * \brief The next IN packet to send on endpoint zero.
 */
extern uint8_t usb_ep0_in_buffer[8];

/**
 * \brief A descriptor for scatter-gather IO.
 */
typedef struct {
	/**
	 * \brief The data.
	 */
	const void *ptr;

	/**
	 * \brief The number of bytes in the data.
	 */
	size_t length;
} usb_iovec_t;

/**
 * \brief The scatter-gather descriptors for the data to send to endpoint zero.
 */
extern usb_iovec_t usb_ep0_data[USB_CONFIG_NUM_EP0_IOVECS];

/**
 * \brief The number of scatter-gather descriptors that apply to the current transfer.
 */
extern uint8_t usb_ep0_data_length;

#endif

