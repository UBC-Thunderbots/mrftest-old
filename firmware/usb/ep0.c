/**
 * \defgroup UEP0 Endpoint zero handling
 *
 * These functions are used to communicate over endpoint zero, the control endpoint.
 * All functions in this section must only be called from the USB stack internal task.
 *
 * @{
 */
#include <usb.h>
#include "internal.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <minmax.h>
#include <stdint.h>
#include <string.h>
#include <task.h>
#include <registers/otg_fs.h>

/**
 * \cond INTERNAL
 */
/**
 * \brief A pointer to the next location where an OUT transaction to endpoint zero will be written by the ISR.
 */
uint8_t *uep0_out_data_pointer = 0;

/**
 * \brief The size of an OUT endpoint zero packet.
 *
 * While the endpoint is enabled, before the transaction is received, this is the maximum number of bytes the ISR is allowed to write into the buffer.
 * After the transaction has been taken, this is the size of the received packet, which may be larger or smaller than the original value.
 */
size_t uep0_out_data_length = 0U;

/**
 * \brief The currently active configuration.
 */
const udev_config_info_t *uep0_current_configuration = 0;

/**
 * \brief The currently active alternate settings.
 */
uint8_t *uep0_alternate_settings = 0;

/**
 * \brief The function that will be invoked after the status stage of the current control transfer.
 */
static void (*uep0_poststatus)(void) = 0;
/**
 * \endcond
 */



/**
 * \cond INTERNAL
 */
static void uep0_enter_configuration(void);
static void uep0_altsetting_activate_endpoints(unsigned int interface, const usb_interface_descriptor_t *interface_descriptor);
static void uep0_altsetting_deactivate_endpoints(const usb_interface_descriptor_t *interface_descriptor);
/**
 * \endcond
 */



/**
 * \cond INTERNAL
 * \brief Waits for an OUT transfer on endpoint zero to finish.
 *
 * This function must be called after the transfer has been enabled in the
 * engine. The function is guaranteed only to return once the endpoint is
 * disabled, or at least in NAK status, one way or another.
 *
 * \retval true if the transfer was successful
 * \retval false if the transfer failed for some reason
 *
 * \exception EPIPE the device state changed
 * \exception ECONNRESET a SETUP transaction arrived
 */
static bool uep0_wait_out(void) {
	for (;;) {
		EventBits_t events = xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED | UEP0_EVENT_SETUP | UEP0_EVENT_OUT_XFRC, pdTRUE, pdFALSE, portMAX_DELAY);
		if ((events & UDEV_EVENT_STATE_CHANGED) && __atomic_load_n(&udev_state, __ATOMIC_RELAXED) != UDEV_STATE_ENUMERATED) {
			// Device state is no longer enumerated due to USB reset, cable unplug, or soft detach.
			if (!(events & (UEP0_EVENT_SETUP | UEP0_EVENT_OUT_XFRC))) {
				// The endpoint has not finished, so it is still enabled. For
				// endpoints other than zero, hardware clears USBAEP on reset,
				// but for endpoint zero, USBAEP is hardwired to 1. We also
				// can’t actually disable the endpoint (EPDIS is read-only in
				// DOEPCTL0). The best we can do is stall further traffic until
				// the next SETUP packet.
				OTG_FS_DOEPCTL0_t ctl = OTG_FS.DOEPCTL0;
				ctl.EPENA = 0;
				ctl.EPDIS = 0;
				ctl.STALL = 1;
				OTG_FS.DOEPCTL0 = ctl;
				// Clear out either of these events which arrived while
				// stalling the endpoint. Otherwise, they might confuse future
				// iterations.
				xEventGroupClearBits(udev_event_group, UEP0_EVENT_SETUP | UEP0_EVENT_OUT_XFRC);
			}
			errno = EPIPE;
			return false;
		}
		if (events & UEP0_EVENT_SETUP) {
			// Setup stage of a new control transfer is finished.
			// We need to leave this bit set in the event mask so the top-level dispatcher will handle the new control transfer.
			xEventGroupSetBits(udev_event_group, UEP0_EVENT_SETUP);
			// Receiving a SETUP packet automatically disables OUT endpoint zero.
			// Therefore, even if the SETUP packet interrupted an ongoing transfer, we don’t need to disable the endpoint ourselves.
			// It’s possible that XFRC might have happened as well.
			// Could this be a problem?
			// Maybe the data stage actually finished before the new SETUP packet arrived, in which case we ought to report the data stage as successful to the caller?
			// Actually, this is not a problem: by sending another SETUP packet, the host omitted the status stage of this transfer.
			// Therefore, clearly, the host no longer cares about this transfer and doesn’t expect any side effects to have occurred.
			// So, it’s safe to discard everything.
			errno = ECONNRESET;
			return false;
		}
		if (events & UEP0_EVENT_OUT_XFRC) {
			// The endpoint has finished.
			return true;
		}
	}
}
/**
 * \endcond
 */

/**
 * \brief Reads the data stage of a control transfer.
 *
 * This function blocks until the data stage finishes or an error is detected.
 *
 * \param[out] buffer the buffer into which to store the received data, which must be large enough to accommodate the length specified in the SETUP packet
 *
 * \retval true if the read completed successfully
 * \retval false if an error occurred before or during the read
 *
 * \exception ECONNRESET another SETUP packet was received since this control transfer started
 * \exception EPIPE the endpoint was disabled, either when the function was first called or while the transfer was occurring, due to USB reset signalling or cable unplug
 * \exception EOVERFLOW the host attempted to send more data than it specified in the SETUP packet
 */
bool uep0_data_read(void *buffer) {
	// Sanity check.
	assert(!udev_setup_packet->bmRequestType.direction);
	assert(udev_setup_packet->wLength);
	assert(buffer);

	// Run the data stage.
	uep0_out_data_pointer = buffer;
	size_t max_packet = udev_info->device_descriptor.bMaxPacketSize0;
	size_t left = udev_setup_packet->wLength;
	while (left) {
		// Enable the endpoint for one packet.
		size_t this = left > max_packet ? max_packet : left;
		uep0_out_data_length = this;
		OTG_FS_DOEPTSIZ0_t tsiz = { .PKTCNT = 1, .XFRSIZ = max_packet };
		OTG_FS.DOEPTSIZ0 = tsiz;
		OTG_FS_DOEPCTL0_t ctl = { .EPENA = 1, .CNAK = 1 };
		OTG_FS.DOEPCTL0 = ctl;

		// Wait until the endpoint finishes, an interrupting SETUP packet arrives, or the device state is no longer enumerated.
		if (!uep0_wait_out()) {
			return false;
		}

		// A packet was shipped.
		// In this case, the ISR sets uep0_out_data_length to the length of the packet actually received, while copying the data into the buffer and advancing the pointer.
		if (uep0_out_data_length > this) {
			errno = EOVERFLOW;
			return false;
		} else {
			left -= uep0_out_data_length;
		}
	}

	// Transfer finished OK.
	return true;
}

/**
 * \cond INTERNAL
 * \brief Waits for an IN transfer on endpoint zero to finish.
 *
 * This function must be called after the transfer has been enabled in the engine.
 * The function is guaranteed only to return once the endpoint is disabled, one way or another.
 *
 * \retval true if the transfer was successful
 * \retval false if the transfer failed for some reason
 *
 * \exception EPIPE the device state changed
 * \exception ECONNRESET a SETUP transaction arrived
 */
static bool uep0_wait_in(void) {
	for (;;) {
		EventBits_t events = xEventGroupWaitBits(udev_event_group, UDEV_EVENT_STATE_CHANGED | UEP0_EVENT_SETUP | UEP0_EVENT_IN_XFRC, pdTRUE, pdFALSE, portMAX_DELAY);
		if ((events & UDEV_EVENT_STATE_CHANGED) && __atomic_load_n(&udev_state, __ATOMIC_RELAXED) != UDEV_STATE_ENUMERATED) {
			// Device state is no longer enumerated due to USB reset, cable unplug, or soft detach.
			if (!(events & (UEP0_EVENT_SETUP | UEP0_EVENT_IN_XFRC))) {
				// The endpoint has not finished, so it is still enabled.
				// We must disable it.
				// To do that, we first establish local NAK.
				xEventGroupClearBits(udev_event_group, UEP0_EVENT_IN_NAKEFF);
				if (!OTG_FS.DIEPCTL0.NAKSTS) {
					OTG_FS.DIEPCTL0.SNAK = 1;
					while (!(xEventGroupWaitBits(udev_event_group, UEP0_EVENT_IN_NAKEFF, pdTRUE, pdFALSE, portMAX_DELAY) & UEP0_EVENT_IN_NAKEFF));
				}
				assert(OTG_FS.DIEPCTL0.NAKSTS);
				// Next, we disable the endpoint, but only if it is currently enabled (disabling a disabled endpoint is documented as being A Bad Idea).
				// There is no race condition between checking EPENA and setting EPDIS, because when NAKSTS=1 there can be no XFRC because no packets can ship.
				// According to the manual, SETUP packet reception sets NAK status but does *not* clear EPENA (though it does for OUT endpoint 0), so that cannot race here.
				if (OTG_FS.DIEPCTL0.EPENA) {
					OTG_FS.DIEPCTL0.EPDIS = 1;
					while (!(xEventGroupWaitBits(udev_event_group, UEP0_EVENT_IN_DISD, pdTRUE, pdFALSE, portMAX_DELAY) & UEP0_EVENT_IN_DISD));
				}
				// Because we might have disabled the endpoint while data was still pending, flush the FIFO.
				udev_flush_tx_fifo(0x80U);
				// Clear out either of these events which arrived while stopping the endpoint.
				// Otherwise, they might confuse future iterations.
				xEventGroupClearBits(udev_event_group, UEP0_EVENT_SETUP | UEP0_EVENT_IN_XFRC);
			}
			errno = EPIPE;
			return false;
		}
		if (events & UEP0_EVENT_SETUP) {
			// Setup stage of a new control transfer is finished.
			// We need to leave this bit set in the event mask so the top-level dispatcher will handle the new control transfer.
			xEventGroupSetBits(udev_event_group, UEP0_EVENT_SETUP);
			// Receiving a SETUP packet automatically sets NAK status on IN endpoint zero.
			// Fully disable the endpoint, but only if it is currently enabled (disabling a disabled endpoint is documented as being A Bad Idea).
			// There is no race condition between checking EPENA and setting EPDIS, because when NAKSTS=1 there can be no XFRC because no packets can ship.
			// According to the manual, SETUP packet reception sets NAK status but does *not* clear EPENA (though it does for OUT endpoint 0), so that cannot race here.
			if (OTG_FS.DIEPCTL0.EPENA) {
				OTG_FS.DIEPCTL0.EPDIS = 1;
				while (!(xEventGroupWaitBits(udev_event_group, UEP0_EVENT_IN_DISD, pdTRUE, pdFALSE, portMAX_DELAY) & UEP0_EVENT_IN_DISD));
			}
			// Because we might have disabled the endpoint while data was still pending, flush the FIFO.
			udev_flush_tx_fifo(0x80U);
			// It’s possible that XFRC might have happened as well.
			// Could this be a problem?
			// Maybe the data stage actually finished before the new SETUP packet arrived, in which case we ought to report the data stage as successful to the caller?
			// Actually, this is not a problem: by sending another SETUP packet, the host omitted the status stage of this transfer.
			// Therefore, clearly, the host no longer cares about this transfer and doesn’t expect any side effects to have occurred.
			// So, it’s safe to discard everything.
			errno = ECONNRESET;
			return false;
		}
		if (events & UEP0_EVENT_IN_XFRC) {
			// The endpoint has finished.
			return true;
		}
	}
}
/**
 * \endcond
 */

/**
 * \brief Writes the data stage of a control transfer.
 *
 * This function blocks until the data has been delivered or an error is detected.
 *
 * \param[in] data the data to send
 *
 * \param[in] length the maximum number of bytes to send, which is permitted to be more or less than the length requested in the SETUP packet (in the former case, the data is truncated)
 *
 * \retval true if the write completed successfully
 * \retval false if an error occurred before or during the write
 *
 * \exception ECONNRESET another SETUP packet was received since this control transfer started, or the host tried to start the status stage after sending less data than specified in the SETUP packet
 * \exception EPIPE the endpoint was disabled, either when the function was first called or while the transfer was occurring, due to USB reset signalling or cable unplug
 */
bool uep0_data_write(const void *data, size_t length) {
	// We consider writing more data than requested to be a successful operation where the data actually delivered is truncated.
	// For uniformity, this policy should apply even when zero bytes are requested.
	// Strictly speaking when wLength=0 a transfer is treated more like an OUT transfer than an IN transfer (the status stage is IN).
	// If an application cares, it can always check the SETUP packet in more detail.
	// However, in the common case where bRequest fully identifies a request, this clause prevents the application from having to special-case wLength=0.
	if (!udev_setup_packet->wLength) {
		return true;
	}

	// Sanity check.
	assert(udev_setup_packet->bmRequestType.direction);
	assert(udev_setup_packet->wLength);
	assert(data || !length);

	// Clamp length to amount requested by host.
	if (length > udev_setup_packet->wLength) {
		length = udev_setup_packet->wLength;
	}

	// Decide whether we will send a ZLP.
	bool need_zlp = (length != udev_setup_packet->wLength) && !(length & (udev_info->device_descriptor.bMaxPacketSize0 - 1U));

	// Run the data stage.
	// DIEPTSIZ0 provides a 2-bit PKTCNT field and a 7-bit XFRSIZ field.
	// That means we can only ship blocks of up to min{3 packets, 127 bytes} at a time.
	// For a 64-byte max packet size, that means one packet at a time.
	// For anything smaller, than means three packets at a time.
	// Because these blocks are so small, rather than having the ISR copy data into the transmit FIFO, we just do it directly on enabling the endpoint.
	// The endpoint 0 transmit FIFO is always long enough for this to be safe.
	//
	// It could occur that the device sends the last data packet of the data stage, but the ACK handshake returned is corrupted.
	// Then the device will still think it’s in the data stage, but the host will think it’s advanced to the status stage.
	// This will result in an attempt to send an OUT transaction while the OUT endpoint is disabled.
	// We fix this by starting the status stage now.
	{
		OTG_FS_DOEPTSIZ0_t tsiz = { .PKTCNT = 1, .XFRSIZ = 0 };
		OTG_FS.DOEPTSIZ0 = tsiz;
		OTG_FS_DOEPCTL0_t ctl = { .EPENA = 1, .CNAK = 1 };
		OTG_FS.DOEPCTL0 = ctl;
	}
	const uint8_t *dptr = data;
	while (length || need_zlp) {
		// Start a block.
		size_t to_push;
		if (length) {
			OTG_FS_DIEPTSIZ0_t tsiz;
			tsiz.PKTCNT = udev_info->device_descriptor.bMaxPacketSize0 == 64U ? 1U : 3U;
			tsiz.XFRSIZ = tsiz.PKTCNT * udev_info->device_descriptor.bMaxPacketSize0;
			if (tsiz.XFRSIZ > length) {
				tsiz.XFRSIZ = length;
			}
			tsiz.PKTCNT = (tsiz.XFRSIZ + udev_info->device_descriptor.bMaxPacketSize0 - 1U) / udev_info->device_descriptor.bMaxPacketSize0;
			OTG_FS.DIEPTSIZ0 = tsiz;
			to_push = tsiz.XFRSIZ;
		} else {
			OTG_FS_DIEPTSIZ0_t tsiz = { .PKTCNT = 1, .XFRSIZ = 0 };
			OTG_FS.DIEPTSIZ0 = tsiz;
			to_push = 0U;
			need_zlp = false;
		}
		OTG_FS_DIEPCTL0_t ctl = { .EPENA = 1, .CNAK = 1, .TXFNUM = 0, .MPSIZ = OTG_FS.DIEPCTL0.MPSIZ };
		OTG_FS.DIEPCTL0 = ctl;
		length -= to_push;

		// The USB engine very strictly requires that all data for a whole packet be pushed into the FIFO at once.
		// This rule even applies ACROSS DIFFERENT FIFOS.
		// Pushing part of a packet, then going and doing some work on a completely different FIFO, then finishing the original packet makes the USB engine fall over and die.
		// For most endpoints, we start (potentially) very large physical transfers and then let the ISR push packets—packet pushing is serialized simply by being in the ISR.
		// For endpoint zero, the small size of PKTCNT/XFRSIZ means large physical transfers are impossible.
		// For a small physical transfer, we save the overhead of getting into the ISR to push data by pushing it immediately when the endpoint is enabled, right here.
		// However, we must ensure the ISR doesn’t fire during this time and go off and push data on some other endpoint.
		// So, we must do this inside a critical section.
		taskENTER_CRITICAL();
		while (to_push) {
			uint32_t word = 0U;
			size_t word_size = to_push;
			if (word_size > sizeof(uint32_t)) {
				word_size = sizeof(uint32_t);
			}
			for (size_t i = 0; i < word_size; ++i) {
				word |= ((uint32_t) *dptr++) << (i * 8U);
			}
			to_push -= word_size;
			OTG_FS_FIFO[0U][0U] = word;
		}
		taskEXIT_CRITICAL();

		// Wait until the endpoint finishes, an interrupting SETUP packet arrives, or the device state is no longer enumerated.
		if (!uep0_wait_in()) {
			return false;
		}
	}

	// Transfer finished OK.
	return true;
}

/**
 * \brief Sets a callback to be called after the status stage completes.
 *
 * \param[in] cb the callback
 */
void uep0_set_poststatus(void (*cb)(void)) {
	uep0_poststatus = cb;
}

/**
 * \cond INTERNAL
 */

/**
 * \brief Runs the status stage for a successful control transfer.
 *
 * This function returns once the status stage has finished.
 * This function is responsible for calling the poststatus callback, if one is registered.
 */
void uep0_status_stage(void) {
	if (!udev_setup_packet->bmRequestType.direction || !udev_setup_packet->wLength) {
		// Data stage was OUT or not present, so status stage will be IN.
		OTG_FS_DIEPTSIZ0_t tsiz = { .PKTCNT = 1, .XFRSIZ = 0 };
		OTG_FS.DIEPTSIZ0 = tsiz;
		OTG_FS_DIEPCTL0_t ctl = { .EPENA = 1, .CNAK = 1, .TXFNUM = 0, .MPSIZ = OTG_FS.DIEPCTL0.MPSIZ };
		OTG_FS.DIEPCTL0 = ctl;

		// Wait until transfer complete.
		uep0_wait_in();
	} else {
		// Data stage was IN so status stage will be OUT.
		// OUT status stages are started in uep0_data_write, so no need to do it here.
		// However, we should still wait until the transfer finishes.
		uep0_wait_out();
	}

	if (uep0_poststatus) {
		uep0_poststatus();
		uep0_poststatus = 0;
	}
}

/**
 * \brief Handles control transfers when no other handler is interested.
 *
 * \param[in] pkt the payload of the SETUP transaction starting the transfer
 *
 * \retval true if the handler accepted the transfer
 * \retval false if the handler could not handle the transfer
 */
bool uep0_default_handler(const usb_setup_packet_t *pkt) {
	static const uint16_t ZERO16 = 0U;
	if (pkt->bmRequestType.type == USB_CTYPE_STANDARD) {
		if (pkt->bmRequestType.recipient == USB_RECIPIENT_DEVICE) {
			if (pkt->bRequest == USB_CREQ_GET_CONFIGURATION && pkt->bmRequestType.direction == 1 && !pkt->wValue && !pkt->wIndex && pkt->wLength == 1U) {
				uint8_t conf = uep0_current_configuration ? uep0_current_configuration->descriptors->bConfigurationValue : 0U;
				uep0_data_write(&conf, sizeof(conf));
				return true;
			} else if (pkt->bRequest == USB_CREQ_GET_DESCRIPTOR && pkt->bmRequestType.direction == 1) {
				const void *descriptor = 0;
				size_t length = 0;
				switch (pkt->wValue >> 8U) {
					case USB_DTYPE_DEVICE:
						if ((pkt->wValue & 0xFFU) == 0) {
							descriptor = &udev_info->device_descriptor;
						}
						break;

					case USB_DTYPE_CONFIGURATION:
						if ((pkt->wValue & 0xFFU) < udev_info->device_descriptor.bNumConfigurations) {
							descriptor = udev_info->configurations[pkt->wValue & 0xFFU]->descriptors;
							length = ((const usb_configuration_descriptor_t *) descriptor)->wTotalLength;
						}
						break;

					case USB_DTYPE_STRING:
						if (!(pkt->wValue & 0xFFU)) {
							descriptor = udev_info->string_zero_descriptor;
						} else if ((pkt->wValue & 0xFFU) <= udev_info->string_count) {
							for (const udev_language_info_t *lang = udev_info->language_table; lang->strings && !descriptor; ++lang) {
								if (lang->id == pkt->wIndex) {
									descriptor = lang->strings[(pkt->wValue & 0xFFU) - 1U];
								}
							}
						}
						break;
				}

				if (descriptor) {
					if (!length) {
						length = *(const uint8_t *) descriptor;
					}
					uep0_data_write(descriptor, length);
					return true;
				}
			} else if (pkt->bRequest == USB_CREQ_GET_STATUS && pkt->bmRequestType.direction == 1 && !pkt->wValue && !pkt->wIndex && pkt->wLength == 2U) {
				uint16_t status = 0U;
				if (__atomic_load_n(&udev_self_powered, __ATOMIC_RELAXED)) {
					status |= 1U;
				}
				uep0_data_write(&status, sizeof(status));
				return true;
			} else if (pkt->bRequest == USB_CREQ_SET_ADDRESS && pkt->wValue <= 127 && !pkt->wIndex && !pkt->wLength) {
				OTG_FS.DCFG.DAD = pkt->wValue;
				return true;
			} else if (pkt->bRequest == USB_CREQ_SET_CONFIGURATION && !pkt->wIndex && !pkt->wLength) {
				const udev_config_info_t *new_config = 0;
				if (pkt->wValue) {
					for (unsigned int i = 0; !new_config && i < udev_info->device_descriptor.bNumConfigurations; ++i) {
						if (udev_info->configurations[i]->descriptors->bConfigurationValue == pkt->wValue) {
							new_config = udev_info->configurations[i];
						}
					}
					if (!new_config) {
						return false;
					}
				}

				if (new_config && new_config->can_enter && !new_config->can_enter()) {
					return false;
				}

				uep0_exit_configuration();
				uep0_current_configuration = new_config;
				uep0_enter_configuration();

				return true;
			}
		} else if (pkt->bmRequestType.recipient == USB_RECIPIENT_INTERFACE && uep0_current_configuration && pkt->wIndex < uep0_current_configuration->descriptors->bNumInterfaces) {
			if (pkt->bRequest == USB_CREQ_GET_INTERFACE && !pkt->wValue && pkt->wLength == 1U) {
				uep0_data_write(&uep0_alternate_settings[pkt->wIndex], 1U);
				return true;
			} else if (pkt->bRequest == USB_CREQ_GET_STATUS && !pkt->wValue && pkt->wLength == 2U) {
				uep0_data_write(&ZERO16, sizeof(ZERO16));
				return true;
			} else if (pkt->bRequest == USB_CREQ_SET_INTERFACE && !pkt->wLength) {
				unsigned int interface = pkt->wIndex;
				unsigned int new_as = pkt->wValue;
				const usb_interface_descriptor_t *newidesc = uutil_find_interface_descriptor(uep0_current_configuration->descriptors, interface, new_as);
				if (newidesc) {
					const udev_interface_info_t *iinfo = uep0_current_configuration->interfaces[interface];
					unsigned int old_as = uep0_alternate_settings[interface];
					const usb_interface_descriptor_t *oldidesc = uutil_find_interface_descriptor(uep0_current_configuration->descriptors, interface, old_as);
					assert(oldidesc);
					if (iinfo && iinfo->alternate_settings[new_as].can_enter && !iinfo->alternate_settings[new_as].can_enter()) {
						return false;
					}
					uep0_altsetting_deactivate_endpoints(oldidesc);
					if (iinfo && iinfo->alternate_settings[old_as].on_exit) {
						iinfo->alternate_settings[old_as].on_exit();
					}
					uep0_alternate_settings[interface] = new_as;
					uep0_altsetting_activate_endpoints(interface, newidesc);
					if (iinfo && iinfo->alternate_settings[new_as].on_enter) {
						iinfo->alternate_settings[new_as].on_enter();
					}
					return true;
				}
			}
		} else if (pkt->bmRequestType.recipient == USB_RECIPIENT_ENDPOINT && UEP_NUM(pkt->wIndex) <= UEP_MAX_ENDPOINT) {
			if (pkt->bRequest == USB_CREQ_CLEAR_FEATURE && !pkt->wLength) {
				if (pkt->wValue == USB_FEATURE_ENDPOINT_HALT) {
					if (!UEP_NUM(pkt->wIndex)) {
						// Endpoint zero is never halted, but always allows
						// halt feature to be cleared.
						return true;
					} else {
						const udev_endpoint_info_t *einfo = uutil_find_endpoint_info(pkt->wIndex);
						if (einfo) {
							return uep_clear_halt(pkt->wIndex, einfo->can_clear_halt, einfo->on_clear_halt);
						}
					}
				}
			} else if (pkt->bRequest == USB_CREQ_GET_STATUS && !pkt->wValue && pkt->wLength == 2U) {
				if (!UEP_NUM(pkt->wIndex)) {
					// Endpoint zero is never halted.
					uep0_data_write(&ZERO16, sizeof(ZERO16));
					return true;
				} else {
					uep_ep_t *ctx = &uep_eps[UEP_IDX(pkt->wIndex)];
					xSemaphoreTake(ctx->mutex, portMAX_DELAY);
					uep_state_t state = ctx->state;
					xSemaphoreGive(ctx->mutex);
					bool exists, halted;
					switch (state) {
						case UEP_STATE_INACTIVE:
							exists = false;
							break;

						case UEP_STATE_IDLE:
						case UEP_STATE_RUNNING:
							exists = true;
							halted = false;
							break;

						case UEP_STATE_HALTED:
						case UEP_STATE_HALTED_WAITING:
							exists = true;
							halted = true;
							break;

						case UEP_STATE_CLEAR_HALT_PENDING:
							exists = true;
							halted = false;
							break;

						default:
							abort();
					}
					if (exists) {
						uint16_t status = halted ? 1U : 0U;
						uep0_data_write(&status, sizeof(status));
						return true;
					}
				}
			} else if (pkt->bRequest == USB_CREQ_SET_FEATURE && !pkt->wLength) {
				if (pkt->wValue == USB_FEATURE_ENDPOINT_HALT && UEP_NUM(pkt->wIndex)) {
					const udev_endpoint_info_t *einfo = uutil_find_endpoint_info(pkt->wIndex);
					if (einfo) {
						uep_halt_with_cb(pkt->wIndex, einfo->on_commanded_halt);
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
 * \brief Enters the current configuration.
 *
 * This function activates all nonzero endpoints.
 * It then invokes the enter callback for the configuration and then the interfaces and alternate settings.
 */
static void uep0_enter_configuration(void) {
	if (uep0_current_configuration) {
		// Configure all the nonzero transmit FIFOs.
		OTG_FS_DIEPTXF0_t txf0 = OTG_FS.DIEPTXF0;
		unsigned int fifo_used = txf0.TX0FSA + txf0.TX0FD;
		for (unsigned int ep = 1U; ep <= UEP_MAX_ENDPOINT; ++ep) {
			OTG_FS_DIEPTXFx_t txf = { .INEPTXFD = MAX(16U, uep0_current_configuration->transmit_fifo_words[ep - 1U]), .INEPTXSA = fifo_used };
			OTG_FS.DIEPTXF[ep - 1U] = txf;
			fifo_used += txf.INEPTXFD;
		}
		assert(fifo_used <= 320U);

		// Scan the descriptor block and activate on all the endpoints.
		const uint8_t *base = (const uint8_t *) uep0_current_configuration->descriptors;
		const uint8_t *dptr = base;
		unsigned int endpoints_to_skip = 0U;
		unsigned int endpoints_in_interface = 0U;
		unsigned int last_interface = UINT_MAX;
		while (dptr - base < uep0_current_configuration->descriptors->wTotalLength) {
			if (dptr[1U] == USB_DTYPE_ENDPOINT) {
				if (endpoints_to_skip) {
					--endpoints_to_skip;
				} else {
					uep_activate((const usb_endpoint_descriptor_t *) dptr, last_interface);
					if (endpoints_in_interface) {
						--endpoints_in_interface;
						if (!endpoints_in_interface) {
							last_interface = UINT_MAX;
						}
					}
				}
			} else if (dptr[1U] == USB_DTYPE_INTERFACE) {
				const usb_interface_descriptor_t *idesc = (const usb_interface_descriptor_t *) dptr;
				if (idesc->bAlternateSetting) {
					// This descriptor describes a non-default alternate setting for an interface.
					// Any endpoint descriptors falling under this interface descriptor only apply when that non-default alternate setting is selected.
					// We should ignore all those endpoint descriptors now because only alternate setting zero of each interface is selected.
					endpoints_to_skip = idesc->bNumEndpoints;
				} else {
					// The next specified number of endpoints will be associated with this interface.
					endpoints_in_interface = idesc->bNumEndpoints;
					last_interface = idesc->bInterfaceNumber;
				}
			}
			dptr += dptr[0U];
		}

		// Now run the configuration entry callback.
		if (uep0_current_configuration->on_enter) {
			uep0_current_configuration->on_enter();
		}

		// Now allocate the alternate settings array, initialize all interfaces to alternate setting zero, and run the alternate setting entry callbacks.
		uep0_alternate_settings = malloc(sizeof(*uep0_alternate_settings) * uep0_current_configuration->descriptors->bNumInterfaces);
		for (unsigned int intf = 0; intf < uep0_current_configuration->descriptors->bNumInterfaces; ++intf) {
			uep0_alternate_settings[intf] = 0U;
			const udev_interface_info_t *iinfo = uep0_current_configuration->interfaces[intf];
			assert(!iinfo->alternate_settings[0U].can_enter);
			if (iinfo && iinfo->alternate_settings[0U].on_enter) {
				iinfo->alternate_settings[0U].on_enter();
			}
		}
	}
}

/**
 * \brief Exits the current configuration.
 *
 * This function deactivates all nonzero endpoints.
 * It then invokes the exit callbacks for all current alternate settings and interfaces and for the configuration itself.
 */
void uep0_exit_configuration(void) {
	if (uep0_current_configuration) {
		// First order of business, scan the descriptor block and deactivate all endpoints.
		const uint8_t *base = (const uint8_t *) uep0_current_configuration->descriptors;
		const uint8_t *dptr = base;
		unsigned int endpoints_to_skip = 0U;
		while (dptr - base < uep0_current_configuration->descriptors->wTotalLength) {
			if (dptr[1U] == USB_DTYPE_ENDPOINT) {
				if (endpoints_to_skip) {
					--endpoints_to_skip;
				} else {
					const usb_endpoint_descriptor_t *edesc = (const usb_endpoint_descriptor_t *) dptr;
					uep_deactivate(edesc);
				}
			} else if (dptr[1U] == USB_DTYPE_INTERFACE) {
				const usb_interface_descriptor_t *idesc = (const usb_interface_descriptor_t *) dptr;
				if (idesc->bAlternateSetting != uep0_alternate_settings[idesc->bInterfaceNumber]) {
					// This descriptor describes an alternate setting that is not currently selected.
					// Any endpoint descriptors falling under this interface descriptor only apply when that alternate setting is selected.
					// We should therefore ignore all those endpoint descriptors.
					endpoints_to_skip = idesc->bNumEndpoints;
				}
			}
			dptr += dptr[0U];
		}

		// Now run the alternate setting exit callbacks.
		for (unsigned int intf = 0; intf < uep0_current_configuration->descriptors->bNumInterfaces; ++intf) {
			uint8_t setting = uep0_alternate_settings[intf];
			const udev_interface_info_t *iinfo = uep0_current_configuration->interfaces[intf];
			if (iinfo && iinfo->alternate_settings[setting].on_exit) {
				iinfo->alternate_settings[setting].on_exit();
			}
		}

		// Next, run the configuration exit callback.
		if (uep0_current_configuration->on_exit) {
			uep0_current_configuration->on_exit();
		}

		// Clean up.
		free(uep0_alternate_settings);
		uep0_alternate_settings = 0;
		uep0_current_configuration = 0;
	}
}

/**
 * \brief Activates the endpoints for an interface alternate setting.
 */
static void uep0_altsetting_activate_endpoints(unsigned int interface, const usb_interface_descriptor_t *interface_descriptor) {
	const uint8_t *dptr = (const uint8_t *) interface_descriptor;
	unsigned int endpoints = interface_descriptor->bNumEndpoints;
	while (endpoints) {
		if (dptr[1U] == USB_DTYPE_ENDPOINT) {
			const usb_endpoint_descriptor_t *edesc = (const usb_endpoint_descriptor_t *) dptr;
			uep_activate(edesc, interface);
			--endpoints;
		}
		dptr += dptr[0U];
	}
}

/**
 * \brief Deactivates the endpoints for an interface alternate setting.
 */
static void uep0_altsetting_deactivate_endpoints(const usb_interface_descriptor_t *interface_descriptor) {
	const uint8_t *dptr = (const uint8_t *) interface_descriptor;
	unsigned int endpoints = interface_descriptor->bNumEndpoints;
	while (endpoints) {
		if (dptr[1U] == USB_DTYPE_ENDPOINT) {
			const usb_endpoint_descriptor_t *edesc = (const usb_endpoint_descriptor_t *) dptr;
			uep_deactivate(edesc);
			--endpoints;
		}
		dptr += dptr[0U];
	}
}
/**
 * \endcond
 */

/**
 * @}
 */

