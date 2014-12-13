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
 * \brief The states that a (non-zero) endpoint can be in.
 */
typedef enum {
	/**
	 * \brief The endpoint is not active in the current configuration or
	 * interface alternate setting.
	 */
	UEP_STATE_INACTIVE,

	/**
	 * \brief The endpoint is ready to use.
	 *
	 * No operation is running. A task may start a transfer.
	 */
	UEP_STATE_IDLE,

	/**
	 * \brief A data transfer is running.
	 *
	 * In the case of a synchronous transfer, \ref uep_read or \ref uep_write
	 * has not returned yet. In the case of an asynchronous transfer, \ref
	 * uep_async_read_finish or \ref uep_async_write_finish has not been
	 * invoked successfully yet.
	 */
	UEP_STATE_RUNNING,

	/**
	 * \brief The endpoint is halted.
	 *
	 * No halt-wait is running. A task may start one.
	 */
	UEP_STATE_HALTED,

	/**
	 * \brief The endpoint is halted and a halt-wait is running.
	 *
	 * In the case of a synchronous halt-wait, \ref uep_halt_wait has not
	 * returned yet. In the case of an asynchronous halt-wait, \ref
	 * uep_async_halt_wait_finish has not been invoked successfully yet.
	 */
	UEP_STATE_HALTED_WAITING,

	/**
	 * \brief The endpoint was halted and a CLEAR FEATURE request was received
	 * clearing the halt feature.
	 *
	 * No halt-wait was running when the request was received, so the
	 * application still needs to start one in order to observe the prior halt
	 * state.
	 */
	UEP_STATE_CLEAR_HALT_PENDING,
} uep_state_t;

/**
 * \ingroup UEP
 * \brief The event numbers that can be delivered via a nonzero endpoint’s \ref
 * uep_ep_t::event_queue "event_queue" member.
 */
typedef enum {
	/**
	 * \brief A physical transfer has finished.
	 *
	 * This event is queued by the ISR when the endpoint is in \ref
	 * UEP_STATE_RUNNING when the physical transfer finishes.
	 */
	UEP_EVENT_XFRC,

	/**
	 * \brief The endpoint has been disabled during a physical transfer.
	 *
	 * This event is queued by the ISR when the endpoint is in \ref
	 * UEP_STATE_RUNNING after the endpoint is fully disabled.
	 */
	UEP_EVENT_EPDISD,

	/**
	 * \brief A halted endpoint was deactivated by a configuration or interface
	 * alternate setting change.
	 *
	 * This event is queued by the internal task when the endpoint is in \ref
	 * UEP_STATE_HALTED_WAITING and is deactivated.
	 */
	UEP_EVENT_DEACTIVATED_WHILE_HALTED,

	/**
	 * \brief A halted endpoint had its halt status cleared.
	 *
	 * This event is queued by the internal task when the endpoint is in \ref
	 * UEP_STATE_HALTED_WAITING and a CLEAR FEATURE request arrives which
	 * successfully clears the endpoint’s halt status.
	 */
	UEP_EVENT_HALT_CLEARED,
} uep_event_t;

/**
 * \ingroup UEP
 * \brief The type of data associated with a nonzero endpoint.
 */
typedef struct {
	/**
	 * \brief A mutex held by the task manipulating the endpoint.
	 *
	 * Not all fields in this structure are protected by the mutex. See details
	 * of individual fields.
	 */
	SemaphoreHandle_t mutex;

	/**
	 * \brief The endpoint’s current state.
	 *
	 * This field is protected by the mutex.
	 */
	uep_state_t state;

	/**
	 * \brief The endpoint’s event queue.
	 *
	 * The queue carries single bytes. Each byte is a member of the \ref
	 * uep_event_t enumeration representing the type of event that occurred.
	 * Events are sent from a variety of sources and always received by the
	 * task that is running a transfer or a halt-wait on the endpoint.
	 *
	 * A task pushing to the queue must hold the mutex. A task blocking on the
	 * queue to pop must not hold the mutex.
	 *
	 * This queue is only one element long. This is adequate, because events
	 * are always used to notify the task currently running a transfer, and
	 * each event terminates the current physical transfer or halt-wait,
	 * requiring service from the application before any subsequent event can
	 * be queued.
	 */
	QueueHandle_t event_queue;

	/**
	 * \brief A semaphore given by the USB interrupt service routine after an
	 * endpoint is disabled due to application request (rather than due to
	 * transfer complete).
	 *
	 * A task blocking on the semaphore must hold the mutex.
	 */
	SemaphoreHandle_t disabled_sem;

	struct {
		/**
		 * \brief Whether a zero-length packet has been received or is pending
		 * to send.
		 *
		 * For OUT endpoints, this indicates whether or not a zero-length
		 * packet was received in the last transfer. For IN endpoints, this
		 * indicates whether or not a zero-length packet needs to be, and has
		 * not yet been, sent at the end of the logical transfer.
		 */
		bool zlp : 1;

		/**
		 * \brief Whether or not the last transfer overflowed.
		 */
		bool overflow : 1;
	}
	/**
	 * \brief Flags regarding the endpoint.
	 *
	 * This field is only touched by the task performing a transfer or by the
	 * ISR. It is not protected by the mutex.
	 */
	flags;

	/**
	 * \brief The interface that contains this endpoint, or UINT_MAX if none.
	 *
	 * This field is only touched by the stack internal task. It is not
	 * protected by the mutex.
	 */
	unsigned int interface;

	union {
		struct {
			/**
			 * \brief A pointer to the start of the receive buffer.
			 */
			uint8_t *buffer;

			/**
			 * \brief A pointer to the next byte to write into.
			 */
			uint8_t *wptr;
		} out;

		struct {
			/**
			 * \brief A pointer to the start of the data to send.
			 */
			const uint8_t *data;

			/**
			 * \brief A pointer to the next byte to read and send.
			 */
			const uint8_t *rptr;
		} in;
	}
	/**
	 * \brief Information about the transfer that is specific to the direction.
	 *
	 * This field is only touched by the task performing a transfer or by the
	 * ISR. It is not protected by the mutex.
	 */
	transfer;

	/**
	 * \brief The number of bytes left in the logical transfer.
	 *
	 * This field is only touched by the task performing a transfer or by the
	 * ISR. It is not protected by the mutex.
	 */
	size_t bytes_left;

	/**
	 * \brief The callback to invoke when the endpoint completes an operation.
	 *
	 * This field is only touched by the task performing a transfer or by the
	 * ISR. It is not protected by the mutex.
	 */
	uep_async_cb_t async_cb;
} uep_ep_t;

/**
 * \endcond
 */



extern EventGroupHandle_t udev_event_group;
extern udev_state_t udev_state;
extern const udev_info_t *udev_info;
extern const usb_setup_packet_t *udev_setup_packet;
extern bool udev_self_powered;
extern SemaphoreHandle_t udev_gonak_mutex;
extern unsigned int udev_gonak_disable_ep;
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
void uep_queue_event(unsigned int ep, uint8_t event);
void uep_queue_event_from_isr(unsigned int ep, uint8_t event, BaseType_t *yield);
bool uep_halt_with_cb(unsigned int ep, void (*cb)(void));
bool uep_clear_halt(unsigned int ep, bool (*cancb)(void), void (*cb)(void));
void uep_activate(const usb_endpoint_descriptor_t *descriptor, unsigned int interface);
void uep_deactivate(const usb_endpoint_descriptor_t *descriptor);

const usb_interface_descriptor_t *uutil_find_interface_descriptor(const usb_configuration_descriptor_t *config, unsigned int interface, unsigned int altsetting);
const udev_endpoint_info_t *uutil_find_endpoint_info(unsigned int ep);

#endif
