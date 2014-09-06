/**
 * \defgroup UDEV Device handling functions
 *
 * \brief These functions handle device-wide state and configuration.
 *
 * An application begins by calling \ref udev_init.
 * This initializes the USB stack and registers the application’s configuration tables.
 * Once the stack is initialized, \ref udev_attach must be called to attach the D+ pull-up resistor and enumerate on the bus to the host.
 *
 * If the application wishes to detach from the bus to simulate a cable unplug without actually unplugging, \ref udev_detach can be called.
 * Once the device is detached, it can be reattached with \ref udev_attach.
 * The device’s configuration tables can also be changed by calling \ref udev_init while detached (it must not be called while attached).
 *
 * Finally, the function \ref udev_isr must be registered in entry 67 of the hardware interrupt vector table to handle OTG_FS interrupts.
 *
 * @{
 */

#include <usb.h>
#include "internal.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <limits.h>
#include <minmax.h>
#include <rcc.h>
#include <semphr.h>
#include <stdbool.h>
#include <string.h>
#include <task.h>
#include <unused.h>
#include <registers/nvic.h>
#include <registers/otg_fs.h>



/**
 * \cond INTERNAL
 */

/**
 * \brief The task handle of the stack internal task.
 */
static TaskHandle_t udev_task_handle;

/**
 * \brief The event group that delivers events from the ISR to the stack internal task.
 */
EventGroupHandle_t udev_event_group;

/**
 * \brief The current state of the device.
 */
udev_state_t udev_state = UDEV_STATE_UNINITIALIZED;

/**
 * \brief The registered configuration table.
 */
const udev_info_t *udev_info;

/**
 * \brief The SETUP transaction of the current control transfer.
 *
 * This variable is null if no control transfer is under way.
 */
const usb_setup_packet_t *udev_setup_packet = 0;

/**
 * \brief A double buffer for holding SETUP transactions.
 *
 * Index \ref udev_setup_packet_wptr will be written into by the ISR whenever a SETUP transaction arrives.
 */
static usb_setup_packet_t udev_setup_packets[2];

/**
 * \brief The index of the buffer which will receive incoming SETUP transactions.
 *
 * The interrupt service routine uses this index to write SETUP transactions into \ref udev_setup_packets.
 * When the setup stage complete event arrives at the stack internal task, it atomically inverts this index to ensure the SETUP transaction is not overwritten while the control transfer is under way.
 */
static unsigned int udev_setup_packet_wptr = 0U; // Index of entry into which new setup packets are written

/**
 * \brief Whether or not the device is currently self-powered.
 */
bool udev_self_powered = false;

/**
 * \brief A mutex that must be held by any task that wishes to take global OUT NAK.
 */
static SemaphoreHandle_t udev_gonak_mutex = 0;

/**
 * \brief A semaphore that is given every time the device enters the detached state.
 */
static SemaphoreHandle_t udev_detach_sem = 0;

/**
 * \brief A mutex that must be held by any task that wishes to use \c OTG_FS.GRSTCTL (for example, to flush a FIFO).
 */
static SemaphoreHandle_t udev_grstctl_mutex = 0;

_Static_assert(INCLUDE_vTaskSuspend == 1, "vTaskSuspend must be included, because otherwise mutex taking can time out!");

/**
 * \endcond
 */



/**
 * \cond INTERNAL
 * \brief The stack internal task.
 */
static void udev_task(void *UNUSED(param)) {
	// The task is created on udev_init, but the device should not attach to the bus until udev_attach.
	// There may be a race condition between initializing the engine and ensuring SDIS is set to not attach to the bus.
	// So, instead, we just wait here and don’t initialize the engine at all until we’re ready to attach for the first time.
	while (__atomic_load_n(&udev_state, __ATOMIC_RELAXED) == UDEV_STATE_DETACHED) {
		xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED, pdTRUE, pdFALSE, portMAX_DELAY);
	}

	// Reset the module and enable the clock
	rcc_enable_reset(AHB2, OTGFS);

	// Reset the USB core and configure device-wide parameters.
	{
		OTG_FS_GUSBCFG_t tmp = {
			.CTXPKT = 0, // This bit should never be set.
			.FDMOD = 1, // Force to device mode (no cable ID used).
			.FHMOD = 0, // Do not force to host mode.
			.TRDT = 5, // Turnaround time 5 PHY clocks to synchronize (there is a formula in the datasheet, but the STM32 library ignores it and just uses 5).
			.HNPCAP = 0, // Not host-negotiation-protocol capable.
			.SRPCAP = 0, // Not session-request-protocol capable.
			.PHYSEL = 1, // This bit is always set.
			.TOCAL = 0, // Do not add additional bit times to interpacket timeout (the STMicro library leaves this at zero; I assume this is fine for the on-chip PHY).
		};
		OTG_FS.GUSBCFG = tmp;
	}
	while (!OTG_FS.GRSTCTL.AHBIDL); // Wait until AHB is idle.
	OTG_FS.GRSTCTL.CSRST = 1; // Core soft reset.
	while (OTG_FS.GRSTCTL.CSRST); // Wait for reset to be complete.
	// Wait:
	// - at least 3 PHY clocks (would be 62.5 ns) after CSRST for the PHY to be ready
	// - at least 25 milliseconds for a change to FDMOD to take effect
	vTaskDelay((25U + portTICK_PERIOD_MS - 1U) / portTICK_PERIOD_MS);
	while (!OTG_FS.GRSTCTL.AHBIDL); // Wait until AHB is idle.
	{
		OTG_FS_GCCFG_t tmp = {
			.NOVBUSSENS = udev_info->flags.vbus_sensing ? 0 : 1, // VBUS sensing may or may not be done.
			.SOFOUTEN = 0, // Do not output SOF pulses to I/O pin.
			.VBUSBSEN = 1, // VBUS sensing B device enabled.
			.VBUSASEN = 0, // VBUS sensing A device disabled.
			.PWRDWN = 1, // Transceiver active.
		};
		OTG_FS.GCCFG = tmp;
	}
	{
		OTG_FS_GAHBCFG_t tmp = {
			.PTXFELVL = 0, // Only used in host mode.
			.TXFELVL = udev_info->flags.minimize_interrupts ? 1 : 0, // Interrupt on TX FIFO half empty.
			.GINTMSK = 1, // Enable interrupts.
		};
		OTG_FS.GAHBCFG = tmp;
	}
	{
		OTG_FS_DCFG_t tmp = OTG_FS.DCFG; // This register may have nonzero reserved bits that must be maintained.
		tmp.DAD = 0, // Address is zero on initial attach.
		tmp.NZLSOHSK = 1, // Send STALL on receipt of non-zero-length status transaction.
		tmp.DSPD = 3, // Run at full speed.
		OTG_FS.DCFG = tmp;
	}
	{
		OTG_FS_GOTGCTL_t tmp = {
			.DHNPEN = 0, // Host negotiation protocol disabled.
			.HSHNPEN = 0, // Host negotiation protocol has not been enabled on the peer.
			.HNPRQ = 0, // Do not issue host negotiation protocol request.
			.SRQ = 0, // Do not issue session request.
		};
		OTG_FS.GOTGCTL = tmp;
	}

	// Globally enable USB interrupts.
	portENABLE_HW_INTERRUPT(67U, udev_info->isr_priority);

	// From this point forward, we are running as a state machine.
	for (;;) {
		// WARNING! Do not turn this loop/switch inside out, such that the stack loops until udev_state changes!
		// This might seem harmless, but would introduce a race condition leading to deadlock when udev_detach() is called from non-stack-internal tasks.
		// Currently, on every signalling of UDEV_EVENT_STATE_CHANGED, if udev_state is in UDEV_STATE_DETACHED, then udev_detach_sem will be given.
		// This will happen even if udev_state didn’t actually change!
		// Foreign-task udev_detach() relies on this.
		switch (__atomic_load_n(&udev_state, __ATOMIC_RELAXED)) {
			case UDEV_STATE_UNINITIALIZED:
				abort();

			case UDEV_STATE_DETACHED:
				// Notify of our arrival here.
				xSemaphoreGive(udev_detach_sem);
				// Wait for someone to call udev_attach.
				xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED, pdTRUE, pdFALSE, portMAX_DELAY);
				break;

			case UDEV_STATE_ATTACHED:
				// Clear the device address.
				OTG_FS.DCFG.DAD = 0U;

				// Wait until:
				// - someone calls udev_detach, which will modify udev_state and give the semaphore
				// - VBUS becomes valid, which causes SRQINT, which will modify udev_state and give the semaphore
				{
					OTG_FS_GINTMSK_t tmp = { .SRQIM = 1 };
					OTG_FS.GINTMSK = tmp;
					xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED, pdTRUE, pdFALSE, portMAX_DELAY);
					tmp.SRQIM = 0;
					OTG_FS.GINTMSK = tmp;
				}
				break;

			case UDEV_STATE_POWERED:
				// Wait until:
				// - someone calls udev_detach, which will modify udev_state and give the semaphore
				// - VBUS becomes invalid, which causes SEDET, which will modify udev_state and give the semaphore
				// - the host issues USB reset signalling, which causes USBRST, which will modify udev_state and give the semaphore
				{
					OTG_FS_GINTMSK_t tmp = { .USBRST = 1, .OTGINT = 1 };
					OTG_FS.GINTMSK = tmp;
					xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED, pdTRUE, pdFALSE, portMAX_DELAY);
					tmp.USBRST = 0;
					tmp.OTGINT = 0;
					OTG_FS.GINTMSK = tmp;
				}
				break;

			case UDEV_STATE_RESET:
				// Initialize and flush receive and endpoint zero transmit FIFOs.
				assert(udev_info->receive_fifo_words >= 16U);
				{
					OTG_FS_GRXFSIZ_t tmp = { .RXFD = udev_info->receive_fifo_words };
					OTG_FS.GRXFSIZ = tmp;
				}
				{
					// See uep0_data_write for why these values.
					OTG_FS_DIEPTXF0_t tmp = { .TX0FSA = udev_info->receive_fifo_words };
					switch (udev_info->device_descriptor.bMaxPacketSize0) {
						case 8U: tmp.TX0FD = 16U; break;
						case 16U: tmp.TX0FD = 16U; break;
						case 32U: tmp.TX0FD = 24U; break;
						case 64U: tmp.TX0FD = 16U; break;
					}
					OTG_FS.DIEPTXF0 = tmp;
				}
				udev_flush_rx_fifo();
				udev_flush_tx_fifo(0x80U);

				// Set local NAK on all OUT endpoints.
				// If this is not done, then on powerup, the endpoints are *disabled*, but they do *not* have local NAK status.
				// In such a situation, an OUT transaction may be ACKed!
				OTG_FS_DOEPCTL0_t ctl0 = { .SNAK = 1 };
				OTG_FS.DOEPCTL0 = ctl0;
				for (unsigned int i = 0U; i < UEP_MAX_ENDPOINT; ++i) {
					OTG_FS_DOEPCTLx_t ctl = { .SNAK = 1 };
					OTG_FS.DOEP[i].DOEPCTL = ctl;
				}

				// Wait until:
				// - someone calls udev_detach, which will modify udev_state and give the semaphore
				// - VBUS becomes invalid, which causes SEDET, which will modify udev_state and give the semaphore
				// - enumeration completes, which causes ENUMDNE, which will modify udev_state and give the semaphore
				{
					OTG_FS_GINTMSK_t tmp = { .ENUMDNEM = 1, .OTGINT = 1 };
					OTG_FS.GINTMSK = tmp;
					xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED, pdTRUE, pdFALSE, portMAX_DELAY);
					tmp.ENUMDNEM = 0;
					tmp.OTGINT = 0;
					OTG_FS.GINTMSK = tmp;
				}
				break;

			case UDEV_STATE_ENUMERATED:
				// Configure endpoint 0 to handle control transfers.
				{
					OTG_FS_DIEPINTx_t tmp = { .INEPNE = 1, .EPDISD = 1, .XFRC = 1, };
					OTG_FS.DIEPINT0 = tmp;
				}
				{
					OTG_FS_DOEPINTx_t tmp = { .EPDISD = 1, };
					OTG_FS.DOEPINT0 = tmp;
				}
				{
					OTG_FS_DIEPCTL0_t tmp = { 0 };
					switch (udev_info->device_descriptor.bMaxPacketSize0) {
						case 8: tmp.MPSIZ = 0b11; break;
						case 16: tmp.MPSIZ = 0b10; break;
						case 32: tmp.MPSIZ = 0b01; break;
						case 64: tmp.MPSIZ = 0b00; break;
						default: abort();
					}
					OTG_FS.DIEPCTL0 = tmp;
				}
				{
					OTG_FS_DOEPCTL0_t tmp = { 0 };
					OTG_FS.DOEPCTL0 = tmp;
				}
				{
					OTG_FS_DAINTMSK_t tmp = { .IEPM = (1U << (UEP_MAX_ENDPOINT + 1U)) - 1U, .OEPM = (1U << (UEP_MAX_ENDPOINT + 1U)) - 1U };
					OTG_FS.DAINTMSK = tmp;
				}

				// Configure applicable interrupts.
				{
					OTG_FS_DIEPMSK_t tmp = { .INEPNEM = 1, .EPDM = 1, .XFRCM = 1, };
					OTG_FS.DIEPMSK = tmp;
				}
				{
					OTG_FS_DOEPMSK_t tmp = { .EPDM = 1, };
					OTG_FS.DOEPMSK = tmp;
				}

				// Clear other endpoints.
				for (unsigned int i = 0; i < UEP_MAX_ENDPOINT * 2U; ++i) {
					uep_eps[i].interface = UINT_MAX;
				}

				// Wait until:
				// - someone calls udev_detach, which will modify udev_state and give the semaphore
				// - VBUS becomes invalid, which causes SEDET, which will modify udev_state and give the semaphore
				// - the host issues USB reset signalling, which causes USBRST, which will modify udev_state and give the semaphore
				{
					OTG_FS_GINTMSK_t tmp = { .OEPINT = 1, .IEPINT = 1, .USBRST = 1, .RXFLVLM = 1, .OTGINT = 1 };
					OTG_FS.GINTMSK = tmp;
				}
				while (__atomic_load_n(&udev_state, __ATOMIC_RELAXED) == UDEV_STATE_ENUMERATED) {
					// Wait for activity.
					EventBits_t events = xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED | UEP0_EVENT_SETUP, pdTRUE, pdFALSE, portMAX_DELAY);
					if (__atomic_load_n(&udev_state, __ATOMIC_RELAXED) == UDEV_STATE_ENUMERATED) {
						if (events & UEP0_EVENT_SETUP) {
							// Setup transaction complete.
							// Grab the SETUP packet.
							udev_setup_packet = &udev_setup_packets[__atomic_fetch_xor(&udev_setup_packet_wptr, 1U, __ATOMIC_RELAXED)];
							__atomic_signal_fence(__ATOMIC_ACQUIRE); // Prevent operations from being hoisted above the udev_setup_packet_wptr fetch and XOR.
							// Go through all available handlers and see who wants this transfer.
							bool handled = false;
							usb_recipient_t recipient = udev_setup_packet->bmRequestType.recipient;
							if ((recipient == USB_RECIPIENT_INTERFACE || recipient == USB_RECIPIENT_ENDPOINT) && uep0_current_configuration) {
								unsigned int interface = UINT_MAX;
								if (recipient == USB_RECIPIENT_INTERFACE) {
									interface = udev_setup_packet->wIndex;
								} else if (recipient == USB_RECIPIENT_ENDPOINT && UEP_NUM(udev_setup_packet->wIndex) <= UEP_MAX_ENDPOINT) {
									interface = uep_eps[UEP_IDX(udev_setup_packet->wIndex)].interface;
								}
								if (interface < uep0_current_configuration->descriptors->bNumInterfaces) {
									const udev_interface_info_t *iinfo = uep0_current_configuration->interfaces[interface];
									if (iinfo) {
										const udev_alternate_setting_info_t *asinfo = &iinfo->alternate_settings[uep0_alternate_settings[interface]];
										handled = handled || (asinfo->control_handler && asinfo->control_handler(udev_setup_packet));
										handled = handled || (iinfo->control_handler && iinfo->control_handler(udev_setup_packet));
									}
								}
							}
							handled = handled || (uep0_current_configuration && uep0_current_configuration->control_handler && uep0_current_configuration->control_handler(udev_setup_packet));
							handled = handled || (udev_info->control_handler && udev_info->control_handler(udev_setup_packet));
							handled = handled || uep0_default_handler(udev_setup_packet);
							if (handled) {
								// The request was accepted and the request handler will have handled any necessary data stage.
								uep0_status_stage();
							} else if (!handled && !(xEventGroupGetBits(udev_event_group) & UEP0_EVENT_SETUP)) {
								// Stall the endpoints and wait for another SETUP packet.
								// Don’t do that if another setup stage has already finished; that would stall the *next* transfer!
								// There’s probably a small race condition around SETUP packets arriving, but the hardware gives us no way to avoid that.
								// It won’t be too bad anyway, as the host should try the transfer three times.
								// Also, the host just plain shouldn’t *be* trying to start another control transfer right after the previous one without doing status first!
								OTG_FS.DOEPCTL0.STALL = 1;
								OTG_FS.DIEPCTL0.STALL = 1;
								if (xEventGroupGetBits(udev_event_group) & UEP0_EVENT_SETUP) {
									if (OTG_FS.DOEPCTL0.STALL || OTG_FS.DIEPCTL0.STALL) {
										// A SETUP packet arrived, but we accidentally stalled after that happened.
										// We can’t usefully handle this control transfer, so just abort it.
										xEventGroupClearBits(udev_event_group, UEP0_EVENT_SETUP);
									}
								}
							}
							udev_setup_packet = 0;
						}
					}
				}
				uep0_exit_configuration();
				{
					OTG_FS_GINTMSK_t tmp = {0};
					OTG_FS.GINTMSK = tmp;
				}
				break;
		}
	}
}

/**
 * \brief Obtains global OUT NAK status.
 */
void udev_gonak_take(void) {
	xSemaphoreTake(udev_gonak_mutex, portMAX_DELAY);
	assert(!(xEventGroupGetBits(udev_event_group) & UDEV_EVENT_GONAKEFF));
	assert(!OTG_FS.GINTSTS.GOUTNAKEFF);
	if (!OTG_FS.GINTSTS.GOUTNAKEFF) {
		assert(!OTG_FS.DCTL.GONSTS);
		OTG_FS.DCTL.SGONAK = 1;
		while (!(xEventGroupWaitBits(udev_event_group, UDEV_EVENT_GONAKEFF, pdTRUE, pdFALSE, portMAX_DELAY) & UDEV_EVENT_GONAKEFF));
	}
	assert(OTG_FS.GINTSTS.GOUTNAKEFF);
	assert(OTG_FS.DCTL.GONSTS);
}

/**
 * \brief Releases global OUT NAK status.
 */
void udev_gonak_release(void) {
	OTG_FS.DCTL.CGONAK = 1;
	assert(!OTG_FS.GINTSTS.GOUTNAKEFF);
	assert(!OTG_FS.DCTL.GONSTS);
	xSemaphoreGive(udev_gonak_mutex);
}

/**
 * \brief Flushes the receive FIFO.
 */
void udev_flush_rx_fifo(void) {
	xSemaphoreTake(udev_grstctl_mutex, portMAX_DELAY);
	while (!OTG_FS.GRSTCTL.AHBIDL);
	OTG_FS_GRSTCTL_t ctl = { .RXFFLSH = 1U };
	OTG_FS.GRSTCTL = ctl;
	while (OTG_FS.GRSTCTL.RXFFLSH);
	while (!OTG_FS.GRSTCTL.AHBIDL);
	xSemaphoreGive(udev_grstctl_mutex);
}

/**
 * \brief Flushes a transmit FIFO.
 *
 * \param[in] ep the endpoint address of the endpoint whose FIFO to flush
 */
void udev_flush_tx_fifo(unsigned int ep) {
	assert(UEP_DIR(ep));
	assert(UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	xSemaphoreTake(udev_grstctl_mutex, portMAX_DELAY);
	while (!OTG_FS.GRSTCTL.AHBIDL);
	OTG_FS_GRSTCTL_t ctl = { .TXFNUM = UEP_NUM(ep), .TXFFLSH = 1U };
	OTG_FS.GRSTCTL = ctl;
	while (OTG_FS.GRSTCTL.TXFFLSH);
	while (!OTG_FS.GRSTCTL.AHBIDL);
	xSemaphoreGive(udev_grstctl_mutex);
}

/**
 * \brief Extracts a packet from the receive FIFO.
 *
 * \param[out] dest the memory location to copy the packet to
 *
 * \param[in] bytes the number of bytes to copy out
 *
 * \param[in] extra the number of bytes left in the FIFO after the packet is copied out, due to overflow, which are extracted and discarded
 */
static void udev_rx_copy_out(void *dest, size_t bytes, size_t extra) {
	uint8_t *pc = dest;
	size_t whole_words = bytes / 4U;
	while (whole_words--) {
		uint32_t word = OTG_FS_FIFO[0U][0U];
		memcpy(pc, &word, sizeof(word));
		pc += sizeof(word);
	}
	bytes %= 4U;
	if (bytes) {
		extra -= MIN(extra, 4U - bytes);
		uint32_t word = OTG_FS_FIFO[0U][0U];
		while (bytes--) {
			*pc++ = (uint8_t) word;
			word >>= 8U;
		}
	}
	extra = (extra + 3U) / 4U;
	while (extra--) {
		(void) OTG_FS_FIFO[0U][0U];
	}
}

/**
 * \brief Writes a packet to a transmit FIFO.
 *
 * \param[out] dest the FIFO to store into
 *
 * \param[in] source the packet to copy in
 *
 * \param[in] bytes the length of the packet in bytes
 */
static void udev_tx_copy_in(volatile uint32_t *dest, const void *source, size_t bytes) {
	const uint8_t *pc = source;
	size_t whole_words = bytes / 4U;
	while (whole_words--) {
		uint32_t word;
		memcpy(&word, pc, sizeof(word));
		*dest = word;
		pc += sizeof(word);
	}
	bytes %= 4U;
	if (bytes) {
		uint32_t word = 0U;
		for (size_t i = 0; i < bytes; ++i) {
			word |= ((uint32_t) pc[i]) << (i * 8U);
		}
		*dest = word;
	}
}
/**
 * \endcond
 */

/**
 * \brief Handles USB interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 67.
 *
 * \internal
 *
 * The general philosophy for handling interrupts in the USB stack is to do the minimum amount of work in the ISR that is sensible, but no less.
 * This leads to the following policy:
 * \li For receive FIFO non-empty interrupts, an element is popped from the FIFO and examined; received packets are copied into their final locations, while other events are marked in event groups for later consideration by tasks.
 * \li For transmit FIFO empty interrupts, data to send is copied from the source buffer into the FIFO, and once the transfer is fully satisfied, the FIFO empty interrupt is masked.
 * \li For state-change interrupts (such as reset, session end, and session request), \ref udev_state is updated immediately and an event is marked for the stack internal task.
 * \li For most other interrupt sources, the interrupt is acknowledged and an event is set in the appropriate event group for later consideration by a task.
 *
 * This policy means that, during normal operation, an entire physical transfer will complete entirely under the ISR’s control before the initiating task is finally woken through its event group to take a transfer complete event.
 * This is a middle ground between ultimate efficiency (wherein the ISR does everything, including decomposing logical transfers into multiple physical transfers and dealing with zero-length packets) and ultimate minimalism (wherein the ISR only ever sets events, and a task must awaken to read or write every single data packet).
 * It also means that the tasks become more closely involved in unusual situations, such as uenxpected disabling of an endpoint; this is acceptable because these unusual situations are not in performance-critical paths and introduce a lot of complexity which does not belong in an ISR.
 */
void udev_isr(void) {
	OTG_FS_GINTMSK_t msk = OTG_FS.GINTMSK;
	OTG_FS_GINTSTS_t sts = OTG_FS.GINTSTS;
	OTG_FS_GOTGINT_t otg = OTG_FS.GOTGINT;
	EventBits_t udev_bits_to_set = 0U;
	EventBits_t oep_bits_to_set[UEP_MAX_ENDPOINT] = { 0U };
	BaseType_t ok;
	BaseType_t yield = pdFALSE;

	// Handle OUT endpoints.
	if (msk.RXFLVLM && sts.RXFLVL) {
		do {
			OTG_FS_GRXSTSR_device_t elt = OTG_FS.GRXSTSP.device;
			switch (elt.PKTSTS) {
				case 0b0001:
					// Global OUT NAK effective.
					udev_bits_to_set |= UDEV_EVENT_GONAKEFF;
					break;

				case 0b0010:
					// OUT data packet received.
					// Copy data into target buffer; do not set any events.
					if (elt.EPNUM) {
						uep_ep_t *ctx = &uep_eps[UEP_IDX(0x00 | elt.EPNUM)];
						if (elt.BCNT) {
							size_t to_copy = MIN(elt.BCNT, ctx->bytes_left);
							udev_rx_copy_out(ctx->data.out, to_copy, elt.BCNT - to_copy);
							ctx->data.out += to_copy;
							ctx->bytes_transferred += to_copy;
							ctx->bytes_left -= to_copy;
						} else {
							ctx->flags.zlp = 1;
						}
					} else {
						// No need to report ZLPs because OUT endpoint 0 should never see a ZLP (except in the status stage, which need not be reported).
						// For data stages, the host should indicate in wLength exactly how much it will send and then send it.
						size_t to_copy = MIN(elt.BCNT, uep0_out_data_length);
						udev_rx_copy_out(uep0_out_data_pointer, to_copy, elt.BCNT - to_copy);
						uep0_out_data_pointer += to_copy;
						uep0_out_data_length = elt.BCNT;
					}
					break;

				case 0b0011:
					// OUT transfer completed.
					if (elt.EPNUM) {
						oep_bits_to_set[elt.EPNUM - 1U] |= UEP_EVENT_XFRC;
					} else {
						udev_bits_to_set |= UEP0_EVENT_OUT_XFRC;
					}
					break;

				case 0b0100:
					// SETUP transaction completed.
					udev_bits_to_set |= UEP0_EVENT_SETUP;
					break;

				case 0b0110:
					// SETUP packet received.
					// Copy data into target buffer; do not set any events.
					assert(elt.EPNUM == 0);
					assert(elt.BCNT == sizeof(usb_setup_packet_t));
					((uint32_t *) &udev_setup_packets[udev_setup_packet_wptr])[0U] = OTG_FS_FIFO[0U][0U];
					((uint32_t *) &udev_setup_packets[udev_setup_packet_wptr])[1U] = OTG_FS_FIFO[0U][0U];
					break;

				default:
					abort();
			}
		} while (OTG_FS.GINTSTS.RXFLVL);
	}
	if (msk.OEPINT && sts.OEPINT) {
		unsigned int epmsk = OTG_FS.DAINT.OEPINT;
		// Endpoint zero never exhibits EPDISD because it can never be disabled under application request.
		for (unsigned int ep = 1U; ep <= UEP_MAX_ENDPOINT; ++ep) {
			if (epmsk & (1U << ep)) {
				OTG_FS_DOEPINTx_t doepint = OTG_FS.DOEP[ep - 1U].DOEPINT;
				if (doepint.EPDISD) {
					OTG_FS_DOEPINTx_t tmp = { .EPDISD = 1 };
					OTG_FS.DOEP[ep - 1U].DOEPINT = tmp;
					oep_bits_to_set[ep - 1U] |= UEP_EVENT_DISD;
				}
			}
		}
	}
	for (unsigned int ep = 1U; ep <= UEP_MAX_ENDPOINT; ++ep) {
		if (oep_bits_to_set[ep - 1U]) {
			uep_ep_t *ctx = &uep_eps[UEP_IDX(0x00 | ep)];
			ok = xEventGroupSetBitsFromISR(ctx->event_group, oep_bits_to_set[ep - 1U], &yield);
			assert(ok);
			if (oep_bits_to_set[ep - 1U] & UEP_EVENT_XFRC) {
				uep_notify_async_from_isr(ctx, &yield);
			}
		}
	}

	// Handle IN endpoints.
	if (msk.IEPINT && sts.IEPINT) {
		unsigned int epint = OTG_FS.DAINT.IEPINT;
		if (epint & 1U) {
			OTG_FS_DIEPINTx_t diepint = OTG_FS.DIEPINT0;
			OTG_FS.DIEPINT0 = diepint;
			if (diepint.INEPNE) {
				udev_bits_to_set |= UEP0_EVENT_IN_NAKEFF;
			}
			if (diepint.EPDISD) {
				udev_bits_to_set |= UEP0_EVENT_IN_DISD;
			}
			if (diepint.XFRC) {
				udev_bits_to_set |= UEP0_EVENT_IN_XFRC;
			}
		}
		OTG_FS_DIEPEMPMSK_t empmsk = OTG_FS.DIEPEMPMSK;
		for (unsigned int ep = 1U; ep <= UEP_MAX_ENDPOINT; ++ep) {
			if (epint & (1U << ep)) {
				OTG_FS_DIEPINTx_t diepint = OTG_FS.DIEP[ep - 1U].DIEPINT;
				uep_ep_t *ctx = &uep_eps[UEP_IDX(0x80 | ep)];

				// Shovel a packet at a time until either:
				// (1) there is not enough space in the FIFO for another packet,
				// (2) the physical transfer is finished
				size_t ep_max_packet = OTG_FS.DIEP[ep - 1U].DIEPCTL.MPSIZ;
				size_t ep_max_packet_words = (ep_max_packet + 3U) / 4U;
				while (ctx->pxfr_bytes_left && OTG_FS.DIEP[ep - 1U].DTXFSTS.INEPTFSAV >= ep_max_packet_words) {
					size_t this_packet_bytes = MIN(ep_max_packet, ctx->pxfr_bytes_left);
					udev_tx_copy_in(&OTG_FS_FIFO[ep][0U], ctx->data.in, this_packet_bytes);
					ctx->bytes_left -= this_packet_bytes;
					ctx->bytes_transferred += this_packet_bytes;
					ctx->pxfr_bytes_left -= this_packet_bytes;
					ctx->data.in += this_packet_bytes;
				}

				// If we are out of data to transfer, stop taking FIFO empty interrupts.
				if (!ctx->pxfr_bytes_left) {
					empmsk.INEPTXFEM &= ~(1U << ep);
				}

				// Check for other interrupt sources.
				EventBits_t to_set = 0U;
				OTG_FS_DIEPINTx_t to_clear = { 0 };
				if (diepint.INEPNE) {
					to_clear.INEPNE = 1;
					to_set |= UEP_IN_EVENT_NAKEFF;
				}
				if (diepint.EPDISD) {
					to_clear.EPDISD = 1;
					to_set |= UEP_EVENT_DISD;
				}
				if (diepint.XFRC) {
					assert(!ctx->pxfr_bytes_left);
					to_clear.XFRC = 1;
					to_set |= UEP_EVENT_XFRC;
				}
				OTG_FS.DIEP[ep - 1U].DIEPINT = to_clear;

				// Post events.
				if (to_set) {
					ok = xEventGroupSetBitsFromISR(ctx->event_group, to_set, &yield);
					assert(ok);
					if (to_set & UEP_EVENT_XFRC) {
						uep_notify_async_from_isr(ctx, &yield);
					}
				}
			}
		}
		OTG_FS.DIEPEMPMSK = empmsk;
	}

	// Handle global, non-endpoint-related activity.
	if (msk.SRQIM && sts.SRQINT) {
		// This interrupt arrives when VBUS rises.
		OTG_FS_GINTSTS_t tmp = { .SRQINT = 1 };
		OTG_FS.GINTSTS = tmp;
		if (udev_state != UDEV_STATE_DETACHED) {
			udev_state = UDEV_STATE_POWERED;
		}
		udev_bits_to_set |= UDEV_EVENT_STATE_CHANGED;
	}
	if (msk.OTGINT && sts.OTGINT) {
		if (otg.SEDET) {
			// This interrupt arrives when VBUS falls.
			OTG_FS_GOTGINT_t tmp = { .SEDET = 1 };
			OTG_FS.GOTGINT = tmp;
			if (udev_state != UDEV_STATE_DETACHED) {
				udev_state = UDEV_STATE_ATTACHED;
			}
			udev_bits_to_set |= UDEV_EVENT_STATE_CHANGED;
		} else {
			abort();
		}
	}
	if (msk.USBRST && sts.USBRST) {
		// This interrupt arrives when USB reset signalling starts.
		OTG_FS_GINTSTS_t tmp = { .USBRST = 1 };
		OTG_FS.GINTSTS = tmp;
		if (udev_state != UDEV_STATE_DETACHED) {
			udev_state = UDEV_STATE_RESET;
		}
		udev_bits_to_set |= UDEV_EVENT_STATE_CHANGED;
		// Ensure a SETUP packet received before the reset won’t get handled after it.
		xEventGroupClearBitsFromISR(udev_event_group, UEP0_EVENT_SETUP);
	}
	if (msk.ENUMDNEM && sts.ENUMDNE) {
		// This interrupt arrives a short time after reset signalling completes.
		OTG_FS_GINTSTS_t tmp = { .ENUMDNE = 1 };
		OTG_FS.GINTSTS = tmp;
		if (udev_state != UDEV_STATE_DETACHED) {
			udev_state = UDEV_STATE_ENUMERATED;
		}
		udev_bits_to_set |= UDEV_EVENT_STATE_CHANGED;
	}

	if (udev_bits_to_set) {
		ok = xEventGroupSetBitsFromISR(udev_event_group, udev_bits_to_set, &yield);
		assert(ok);
	}

	if (yield) {
		portYIELD_FROM_ISR();
	}
}

/**
 * \brief Initializes the USB subsystem.
 *
 * This must be the first USB stack function the application calls.
 *
 * This function can safely be called before the FreeRTOS scheduler is started.
 *
 * \param[in] info the device configuration table, which must remain valid for the entire time the subsystem is enabled
 *
 * \pre The subsystem must be uninitialized or the device must be detached from the bus.
 *
 * \post The USB subsystem is initialized with the device soft-detached from the bus.
 */
void udev_init(const udev_info_t *info) {
	assert(info);

	switch (__atomic_load_n(&udev_state, __ATOMIC_RELAXED)) {
		case UDEV_STATE_UNINITIALIZED:
			// Initialize the stack.
			udev_info = info;
			udev_state = UDEV_STATE_DETACHED;
			if (!(udev_event_group = xEventGroupCreate())) {
				abort();
			}
			if (!(udev_gonak_mutex = xSemaphoreCreateMutex())) {
				abort();
			}
			if (!(udev_detach_sem = xSemaphoreCreateBinary())) {
				abort();
			}
			if (!(udev_grstctl_mutex = xSemaphoreCreateMutex())) {
				abort();
			}
			for (unsigned int i = 0; i < UEP_MAX_ENDPOINT * 2U; ++i) {
				uep_eps[i].interface = UINT_MAX;
				if (!(uep_eps[i].event_group = xEventGroupCreate())) {
					abort();
				}
				if (!(uep_eps[i].transfer_mutex = xSemaphoreCreateMutex())) {
					abort();
				}
				if (!(uep_eps[i].manage_mutex = xSemaphoreCreateMutex())) {
					abort();
				}
				xEventGroupSetBits(uep_eps[i].event_group, UEP_EVENT_DEACTIVATED | UEP_EVENT_NOT_HALTED);
				uep_eps[i].pxfr_bytes_left = 0U;
			}
			if (!xTaskCreate(&udev_task, "usb", info->internal_task_stack_size, 0, info->internal_task_priority, &udev_task_handle)) {
				abort();
			}
			break;

		case UDEV_STATE_DETACHED:
			// Only change the configuration table and a few global control flags.
			udev_info = info;
			OTG_FS.GCCFG.NOVBUSSENS = info->flags.vbus_sensing ? 0U : 1U;
			OTG_FS.GAHBCFG.TXFELVL = info->flags.minimize_interrupts ? 1U : 0U;
			break;

		default:
			abort();
	}
}

/**
 * \brief Attaches to the bus.
 *
 * This function can safely be called before the FreeRTOS scheduler is started.
 *
 * \pre The subsystem must be initialized.
 *
 * \pre The device must be detached from the bus.
 *
 * \post The device is attached to the bus and will exhibit a pull-up resistor on D+ when VBUS is present (or always, if \ref udev_info_t::vbus_sensing is zero).
 */
void udev_attach(void) {
	udev_state_t old = __atomic_exchange_n(&udev_state, UDEV_STATE_ATTACHED, __ATOMIC_RELAXED);
	assert(old == UDEV_STATE_DETACHED);
	OTG_FS.DCTL.SDIS = 0;
	xEventGroupSetBits(udev_event_group, UDEV_EVENT_STATE_CHANGED);
}

/**
 * \brief Detaches from the bus.
 *
 * This function blocks, only returning once the detach operation is complete.
 *
 * Note that this function will invoke the exit callbacks for the active configuration and all interface alternate settings.
 * Therefore, those callbacks must be safely invokable from the point at which this function is called.
 * This must therefore not be called from a configuration or interface alternate setting enter/exit callback.
 * It must also normally not be called from a task performing nonzero endpoint operations, as the exit callback would deadlock waiting for the task to terminate.
 *
 * All endpoints are deactivated as a result of this call.
 *
 * \pre The subsystem must be initialized.
 *
 * \pre The device must be attached to the bus.
 *
 * \pre This function must not already be running.
 *
 * \post The device is detached from the bus and does not exhibit a pull-up resistor on D+.
 *
 * \post Exit callbacks for all previously active configurations and interface alternate settings have completed.
 */
void udev_detach(void) {
	while (xSemaphoreTake(udev_detach_sem, 0U));
	udev_state_t old = __atomic_exchange_n(&udev_state, UDEV_STATE_DETACHED, __ATOMIC_RELAXED);
	assert(old != UDEV_STATE_UNINITIALIZED && old != UDEV_STATE_DETACHED);
	xEventGroupSetBits(udev_event_group, UDEV_EVENT_STATE_CHANGED);
	if (xTaskGetCurrentTaskHandle() == udev_task_handle) {
		uep0_exit_configuration();
	} else {
		xSemaphoreTake(udev_detach_sem, portMAX_DELAY);
	}
	OTG_FS.DCTL.SDIS = 1;
	vTaskDelay(1U);
}

/**
 * \brief Sets whether the device is currently self-powered.
 *
 * A self-powered device is one that is receiving power from a source other than USB VBUS.
 * Because power sources can be attached and detached dynamically, the application must keep the USB stack up-to-date on whether external power is available.
 * By default, if this function is never called, a device is assumed to be bus-powered.
 * If the device is self-powered at boot, this function should be called after \ref udev_init but before \ref udev_attach.
 *
 * This function can be called at absolutely any time, including from an interrupt service routine.
 *
 * \param[in] sp \c true if an external power source is attached, or \c false if all power is currently coming from USB VBUS
 */
void udev_set_self_powered(bool sp) {
	__atomic_store_n(&udev_self_powered, sp, __ATOMIC_RELAXED);
}

/**
 * @}
 */

