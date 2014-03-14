#ifndef USB_USB_INTERNAL_H
#define USB_USB_INTERNAL_H

#include <FreeRTOS.h>
#include <event_groups.h>
#include <semphr.h>
#include <stdbool.h>
#include <stdint.h>



/**
 * \cond INTERNAL
 */

/**
 * \ingroup UDEV
 * \brief The event bits that can be delivered via \ref udev_event_group.
 *
 * Because the stack internal task handles both device state events and endpoint zero traffic, both types of events are present here.
 */
typedef enum {
	/**
	 * \brief Indicates that global OUT NAK has taken effect and all prior OUT and SETUP activity has been drained and delivered.
	 *
	 * This flag is set by the ISR when the global OUT NAK effective pattern appears in the receive status FIFO.
	 * It is atomically cleared by the task that is waiting for global OUT NAK status as part of resuming that task.
	 */
	UDEV_EVENT_GONAKEFF      = 0x000001,

	/**
	 * \brief Indicates that \ref udev_state has changed value.
	 *
	 * This flag is set by any code which modifies \ref udev_state.
	 * This is the ISR in response to a session request, session end, USB reset, or enumeration complete interrupt; and \ref udev_attach and \ref udev_detach.
	 * It is atomically cleared by the stack internal task while waiting for a state change or completion of an endpoint zero transfer component, as part of resuming that task.
	 */
	UDEV_EVENT_STATE_CHANGED = 0x000002,

	/**
	 * \brief Indicates that the setup stage of a control transfer has finished.
	 *
	 * This flag is usually set by the ISR when a SETUP stage completes.
	 * It is usually atomically cleared by the stack internal task while waiting for a control transfer to start, as part of resuming that task.
	 *
	 * In some cases, this flag may be atomically cleared by code elsewhere in the stack internal task which needs to wake on a SETUP transaction but which does not directly handle it.
	 * Such code sets the flag immediately after waking up, so that the outer loop of the stack internal task will handle the event.
	 *
	 * Finally, the bit may be cleared in the ISR or stack internal task in cases where a SETUP transaction has occurred but other activity makes it inappropriate to handle it.
	 */
	UEP0_EVENT_SETUP         = 0x000004,

	/**
	 * \brief Indicates that an OUT component of a transfer on endpoint zero has finished.
	 *
	 * This flag is set by the ISR when the transfer complete pattern appears in the receive status FIFO.
	 * It is atomically cleared by the stack internal task while waiting for the OUT component to complete, as part of resuming that task.
	 */
	UEP0_EVENT_OUT_XFRC      = 0x000008,

	/**
	 * \brief Indicates that an IN component of a transfer on endpoint zero has finished.
	 *
	 * This flag is set by the ISR when the transfer complete interrupt is asserted.
	 * It is atomically cleared by the stack internal task while waiting for the IN component to complete, as part of resuming that task.
	 */
	UEP0_EVENT_IN_XFRC       = 0x000010,

	/**
	 * \brief Indicates that IN endpoint zero’s NAK status bit has become set.
	 *
	 * This flag is set by the ISR when the NAK effective interrupt is asserted.
	 * It is cleared by the stack internal task before requesting local NAK status, then waited on.
	 */
	UEP0_EVENT_IN_NAKEFF     = 0x000020,

	/**
	 * \brief Indicates that IN endpoint zero has been disabled due to firmware request (not due to transfer completion).
	 *
	 * This flag is set by the ISR when the endpoint disabled interrupt is asserted.
	 * It is atomically cleared by the stack internal task while waiting for the endpoint to disable, as part of resuming that task.
	 */
	UEP0_EVENT_IN_DISD       = 0x000040,
} udev_event_bits_t;

/**
 * \ingroup UDEV
 * \brief The possible states the device can be in.
 */
typedef enum {
	/**
	 * \brief The subsystem has not yet been initialized.
	 */
	UDEV_STATE_UNINITIALIZED,

	/**
	 * \brief The device is detached from the bus and will not exhibit a D+ pull-up resistor.
	 */
	UDEV_STATE_DETACHED,

	/**
	 * \brief The device is attached to the bus but is not yet exhibiting a D+ pull-up resistor because VBUS is not yet valid.
	 */
	UDEV_STATE_ATTACHED,

	/**
	 * \brief The device is attached and has observed a valid level on VBUS and is awaiting USB device reset signalling.
	 */
	UDEV_STATE_POWERED,

	/**
	 * \brief Device reset signalling has been received, but enumeration complete has not yet been observed.
	 */
	UDEV_STATE_RESET,

	/**
	 * \brief Enumeration complete has occurred and the device is ready to transfer packets.
	 */
	UDEV_STATE_ENUMERATED,
} udev_state_t;



/**
 * \ingroup UEP
 * \brief The event bits that can be delivered via a nonzero endpoint’s \ref uep_ep_t::event_group "event_group" member.
 */
typedef enum {
	/**
	 * \brief Indicates that the endpoint has been disabled due to firmware request (not due to transfer completion).
	 *
	 * This flag is set by the ISR when the endpoint disabled interrupt is asserted.
	 * It is atomically cleared by the task running a transfer on the endpoint which triggered disablement, as part of resuming that task.
	 * It is also cleared while activating an endpoint, in case old data was left over.
	 */
	UEP_EVENT_DISD = 0x000001,

	/**
	 * \brief Indicates that local NAK status has become effective for the endpoint.
	 *
	 * This flag is only used for IN endpoints.
	 *
	 * This flag is set by the ISR when the NAK effective interrupt is asserted.
	 * It is atomically cleared by the task running a transfer on the endpoint which triggered local NAK, as part of resuming that task.
	 * It is also cleared while activating an endpoint, in case old data was left over.
	 */
	UEP_IN_EVENT_NAKEFF = 0x000002,

	/**
	 * \brief Indicates that a transfer has finished.
	 *
	 * This flag is set by the ISR when the transfer complete pattern appears in the receive status FIFO (for OUT endpoints) or the transfer complete interrupt is asserted (for IN endpoints).
	 * It is atomically cleared by the task running a transfer on the endpoint, as part of resuming that task (or polling, for the asynchronous API).
	 * It is also cleared while activating an endpoint, in case old data was left over.
	 */
	UEP_EVENT_XFRC = 0x000004,

	/**
	 * \brief Indicates that the endpoint is, or is being, deactivated.
	 *
	 * This flag is set for all (nonzero) endpoints at stack initialization time, because all endpoints are initially deactivated.
	 * It is also set when an endpoint is disabled due to activity on endpoint zero.
	 *
	 * It is generally cleared when the endpoint is enabled due to activity on endpoint zero.
	 *
	 * In some cases, tasks running transfers may unintentionally clear this flag while waiting or polling; they then re-set the flag in such cases.
	 */
	UEP_EVENT_DEACTIVATED = 0x000008,

	/**
	 * \brief Indicates that the endpoint is halted.
	 *
	 * This flag is set when an endpoint is halted due to application calling \ref uep_halt or due to a SET FEATURE request on endpoint zero.
	 * It is cleared due to a CLEAR FEATURE request on endpoint zero.
	 * It is also cleared when an endpoint is activated, because endpoints are initially not halted when activated.
	 *
	 * In some cases, tasks running transfers may unintentionally clear this flag while waiting or polling; they then re-set the flag in such cases.
	 */
	UEP_EVENT_HALTED = 0x000010,

	/**
	 * \brief Indicates that the endpoint is not halted.
	 *
	 * This flag is set for all (nonzero) endpoints at stack initialization time, because no endpoints are initially halted.
	 * It is also set due to a CLEAR FEATURE request on endpoint zero.
	 * It is also set when an endpoint is activated, because endpoints are initially not halted when activated.
	 * It is cleared when an endpoint is halted due to \ref uep_halt or SET FEATURE.
	 */
	UEP_EVENT_NOT_HALTED = 0x000020,
} uep_out_event_bits_t;

/**
 * \ingroup UEP
 * \brief The type of data associated with a nonzero endpoint.
 */
typedef struct {
	struct {
		/**
		 * \brief Whether a zero-length packet has been received or is pending to send.
		 *
		 * For OUT endpoints, this indicates whether or not a zero-length packet was received in the last transfer.
		 * For IN endpoints, this indicates whether or not a zero-length packet needs to be, and has not yet been, sent at the end of the logical transfer.
		 */
		unsigned zlp : 1;

		/**
		 * \brief Whether or not the last transfer overflowed.
		 */
		unsigned overflow : 1;
	}
	/**
	 * \brief Flags regarding the endpoint.
	 */
	flags;

	/**
	 * \brief The interface that contains this endpoint, or UINT_MAX if none.
	 */
	unsigned int interface;

	/**
	 * \brief An event group for delivering events related to this endpoint.
	 */
	EventGroupHandle_t event_group;

	/**
	 * \brief A mutex held by the task that is currently running a read or write operation or that needs to prevent one from starting.
	 */
	SemaphoreHandle_t transfer_mutex;

	/**
	 * \brief A mutex held by the task that is currently setting or clearing endpoint halt status or activating or deactivating the endpoint.
	 *
	 * A task may take this mutex, then signal the endpoint via \ref event_group, then take \ref transfer_mutex, in order to ensure the endpoint is idle.
	 * This mutex must not be taken if \ref transfer_mutex is already held, to avoid deadlock.
	 */
	SemaphoreHandle_t manage_mutex;

	union {
		uint8_t *out;
		const uint8_t *in;
	}
	/**
	 * \brief A pointer to the data buffer.
	 */
	data;

	/**
	 * \brief The number of bytes left in the current logical transfer.
	 */
	size_t bytes_left;

	/**
	 * \brief The number of bytes received so far in the current logical transfer.
	 */
	size_t bytes_transferred;

	/**
	 * \brief The number of bytes left to feed to the FIFO for the current physical transfer, for IN endpoints.
	 */
	size_t pxfr_bytes_left;

	/**
	 * \brief The asynchronous API event group to notify when this endpoint needs servicing.
	 */
	EventGroupHandle_t async_group;

	/**
	 * \brief The bits to set in \ref async_group when this endpoint needs servicing.
	 */
	EventBits_t async_bits;
} uep_ep_t;

/**
 * \endcond
 */



extern EventGroupHandle_t udev_event_group;
extern udev_state_t udev_state;
extern const udev_info_t *udev_info;
extern const usb_setup_packet_t *udev_setup_packet;
extern bool udev_self_powered;
void udev_gonak_take(void);
void udev_gonak_release(void);
void udev_flush_rx_fifo(void);
void udev_flush_tx_fifo(unsigned int ep);

extern uint8_t *uep0_out_data_pointer;
extern size_t uep0_out_data_length;
extern const udev_config_info_t *uep0_current_configuration;
extern uint8_t *uep0_alternate_settings;
void uep0_status_stage(void);
bool uep0_default_handler(const usb_setup_packet_t *pkt);
void uep0_exit_configuration(void);

extern uep_ep_t uep_eps[UEP_MAX_ENDPOINT * 2U];
void uep_notify_async(uep_ep_t *ctx);
void uep_notify_async_from_isr(uep_ep_t *ctx, BaseType_t *yield);
void uep_activate(const usb_endpoint_descriptor_t *descriptor, unsigned int interface);
void uep_deactivate(const usb_endpoint_descriptor_t *descriptor);
void uep_get_async_event(unsigned int ep, EventGroupHandle_t *async_group, EventBits_t *async_bits);

const usb_interface_descriptor_t *uutil_find_interface_descriptor(const usb_configuration_descriptor_t *config, unsigned int interface, unsigned int altsetting);
const udev_endpoint_info_t *uutil_find_endpoint_info(unsigned int ep);

#endif

