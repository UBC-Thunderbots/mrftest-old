#ifndef STM32LIB_USB_LL_H
#define STM32LIB_USB_LL_H

#include <registers/otg_fs.h>
#include <stdbool.h>

/**
 * \file
 *
 * \brief Provides low-level management of the USB engine in device mode.
 *
 * This module provides a thin wrapper around the USB engine with hooks to attach higher-level functionality.
 * This module handles attaching and detaching on the bus, USB reset signalling, and demultiplexing and dispatching per-endpoint interrupts and receive FIFO patterns.
 *
 * The general operating model is one of callbacks.
 * The application initializes the USB engine, causing it to attach to the bus.
 * As part of initialization, the application provides a callback which is invoked when USB reset occurs.
 * Once USB reset occurs, the application initializes endpoint registers to handle traffic.
 * It also attaches callbacks to handle interrupts (in the case of IN endpoints) or receive FIFO patterns (in the case of OUT endpoints).
 * This module automatically invokes those callbacks as the relevant endpoints see interrupts or as receive FIFO status patterns appear for those endpoints.
 */



/**
 * \brief The possible states a device can be in.
 */
typedef enum {
	/**
	 * \brief The device is not attached to the bus; to attach, \ref usb_ll_attach must be invoked.
	 */
	USB_LL_STATE_DETACHED,

	/**
	 * \brief The device is attached but has not yet seen USB reset signalling; no endpoints should be enabled.
	 */
	USB_LL_STATE_POWERED,

	/**
	 * \brief The device has seen USB reset signalling but not yet finished speed enumeration; no endpoints should be enabled.
	 */
	USB_LL_STATE_ENUMERATING,

	/**
	 * \brief The device has seen a USB reset and is ready to communicate (perhaps on address zero or perhaps on a nonzero address; this module does not track address assignment).
	 */
	USB_LL_STATE_ACTIVE,
} usb_ll_state_t;

/**
 * \brief The type of a callback invoked when a USB reset event occurs.
 *
 * The reset callback is invoked under global NAK and should:
 * \li tear down any ongoing activity on the USB
 * \li disable all endpoints (including endpoint zero), and
 * \li configure the sizes of the receive and transmit zero FIFOs.
 *
 * During invocation of the reset callback, the device’s current state still holds its old value; thus, the callback can check for \ref USB_LL_STATE_ACTIVE to decide whether or not it needs to tear down application activity.
 * After the callback returns, the state machine switches to \ref USB_LL_STATE_ENUMERATING.
 *
 * \pre The device is in one of \ref USB_LL_STATE_POWERED, \ref USB_LL_STATE_ENUMERATING, or \ref USB_LL_STATE_ACTIVE.
 */
typedef void (*usb_ll_reset_cb_t)(void);

/**
 * \brief The type of callback invoked when a USB speed enumeration complete event occurs.
 *
 * The enumeration complete callback is \em not invoked under global NAK.
 * It should set up endpoint zero to handle control requests from the host.
 * After the callback returns, the state machine switches to \ref USB_LL_STATE_ACTIVE.
 *
 * \pre The device is in \ref USB_LL_STATE_ENUMERATING.
 */
typedef void (*usb_ll_enumeration_done_cb_t)(void);

/**
 * \brief The type of callback invoked when the user unplugs the USB cable.
 *
 * This callback is \em not invoked under global NAK.
 *
 * The application should typically do the following when this event occurs:
 * \li request a global NAK and perform the remaining steps under global NAK
 * \li tear down application activity, if the device was in \ref USB_LL_STATE_ACTIVE,
 * \li disable all endpoints (including endpoint zero), and
 * \li call \ref usb_ll_detach
 */
typedef void (*usb_ll_unplug_cb_t)(void);

/**
 * \brief The type of a callback that handles IN endpoint interrupts.
 *
 * The callback is expected to clear at least one pending interrupt condition towards allowing the endpoint’s interrupt status to clear.
 *
 * \param ep the endpoint number
 */
typedef void (*usb_ll_in_cb_t)(unsigned int ep);

/**
 * \brief The type of a callback that handles OUT endpoint patterns.
 *
 * \param ep the endpoint number
 *
 * \param pattern the pattern to handle
 */
typedef void (*usb_ll_out_cb_t)(unsigned int ep, OTG_FS_GRXSTSR_device_t pattern);

/**
 * \brief The type of callback the application provides to be notified when global NAK occurs.
 */
typedef void (*usb_ll_gnak_cb_t)(void);

/**
 * \brief A request for global NAK.
 *
 * The application must allocate a structure of this type in order to issue a global NAK request.
 * The application should not modify the elements of this structure.
 * The allocated structure should be initialized to \ref USB_LL_GNAK_REQ_INIT.
 */
typedef struct usb_ll_gnak_req {
	struct usb_ll_gnak_req *next;
	bool queued;
	usb_ll_gnak_cb_t cb;
} usb_ll_gnak_req_t;

/**
 * \brief The initialization value for a \ref usb_ll_gnak_req_t.
 */
#define USB_LL_GNAK_REQ_INIT { 0, false, 0 }



/**
 * \name Device states
 *
 * These functions deal with the device as a whole moving between different states.
 *
 * @{
 */

/**
 * \brief Returns the device’s current state.
 *
 * Unlike the other functions, this function can be invoked at any time, even when the device is detached.
 *
 * \return the current state
 */
usb_ll_state_t usb_ll_get_state(void);

/**
 * \brief Initializes the device and attaches to the bus.
 *
 * While this function will enable all interrupt-handling machinery inside the USB core itself, it will not configure the NVIC.
 * The application must enable the interrupt in the NVIC if it wishes to use interrupts to handle USB activity.
 *
 * \pre The device must be in \ref USB_LL_STATE_DETACHED.
 *
 * \param reset_cb the callback to invoke when USB reset signalling occurs; this may be null but that is unlikely to be useful
 *
 * \param enumeration_done_cb the callback to invoke when speed enumeration finishes after a reset; this may be null but that is unlikely to be useful
 *
 * \param unplug_cb the callback to invoke when the device is unplugged; this may be null and that is reasonable in the case of a bus-powered device
 */
void usb_ll_attach(usb_ll_reset_cb_t reset_cb, usb_ll_enumeration_done_cb_t enumeration_done_cb, usb_ll_unplug_cb_t unplug_cb);

/**
 * \brief Detaches from the bus.
 *
 * Applications take global NAK callbacks in order of registration.
 * However, after detaching, it likely does not make sense for any other global NAK callbacks that might have been registered to be taken.
 * Therefore, it is recommended that whatever callback invokes this function ensure it is the \em last global NAK callback registered.
 * It can do this by registering a global NAK request, allowing it to be delivered, then, from the global NAK callback, registering a \em second global NAK request.
 * That second request is guaranteed to be taken after any other requests, and no further requests can be registered since no further activity will occur as global NAK is already in force.
 *
 * \pre The device must be in \ref USB_LL_STATE_POWERED, \ref USB_LL_STATE_ENUMERATING, or \ref USB_LL_STATE_ACTIVE.
 *
 * \pre All application activity must be torn down and all endpoints (including endpoint zero) must be disabled.
 *
 * \pre Global NAK must be effective.
 *
 * \post The device is in \ref USB_LL_STATE_DETACHED.
 */
void usb_ll_detach(void);

/**
 * @}
 */



/**
 * \name Endpoint activity handling
 *
 * These functions deal with demultiplexing and dispatching notifications of activity occurring on specific endpoints.
 *
 * @{
 */

/**
 * \brief Attaches a callback to handle IN endpoint interrupts.
 *
 * \param ep the endpoint number
 *
 * \param cb the callback to register
 */
void usb_ll_in_set_cb(unsigned int ep, usb_ll_in_cb_t cb);

/**
 * \brief Attaches a callback to handle OUT endpoint receive FIFO patterns.
 *
 * \param ep the endpoint number
 *
 * \param cb the callback to register, which accepts the FIFO pattern
 */
void usb_ll_out_set_cb(unsigned int ep, usb_ll_out_cb_t cb);

/**
 * @}
 */



/**
 * \name Global NAK handling
 *
 * These functions allow an application to enter global NAK mode, where endpoints can be manipulated without worrying about bus transactions occuring simultaneously.
 * Global NAK mode also serves as a serializing barrier point in the receive FIFO—because no bus activity occurs after global NAK, the receive FIFO is guaranteed to be, and remain, empty.
 *
 * @{
 */

/**
 * \brief Requests for global NAK to occur.
 *
 * The USB stack will enter global NAK state as soon as possible and will then invoke the provided callback.
 * Global NAK state will end after the callback returns.
 *
 * It is safe to call this function with a request structure that has already been queued.
 * In this case, the associated callback is \em changed to the one specified in the subsequent call.
 *
 * \param req the request structure identifying this request
 *
 * \param cb the callback to invoke when global NAK becomes effective, which may be null to effectively cancel an already-queued request
 */
void usb_ll_set_gnak(usb_ll_gnak_req_t *req, usb_ll_gnak_cb_t cb);

/**
 * @}
 */



/**
 * \name Interrupt/activity processing
 *
 * This function allows the application to grant time to the USB engine to handle events.
 *
 * @{
 */

/**
 * \brief Handles USB activity.
 *
 * This function \em must be called quickly if a USB interrupt occurs.
 * It \em may be harmlessly called when a USB interrupt does not occur.
 *
 * All USB callbacks will be invoked as a consequence of, and in the same context (e.g. interrupt level) as, this function.
 */
void usb_ll_process(void);

/**
 * @}
 */

#endif

