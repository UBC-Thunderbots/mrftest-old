#include "usb_ep0.h"
#include "registers.h"
#include "usb.h"
#include "usb_ep0_sources.h"
#include "usb_internal.h"

static const usb_ep0_global_callbacks_t *global_callbacks = 0;
static const usb_ep0_configuration_callbacks_t * const *configuration_callbacks = 0;
static const usb_ep0_endpoints_callbacks_t *endpoints_callbacks = 0;
static uint8_t setup_packet[8];
static usb_ep0_source_t *data_source = 0;
static uint8_t *data_sink = 0;
static size_t data_requested = 0;
static bool (*out_complete_callback)(void) = 0;
static uint8_t current_configuration = 0;
static const usb_ep0_configuration_callbacks_t *current_configuration_callbacks = 0;

static bool push_transaction(void) {
	// If the host isn’t expecting us to send any data, we’re finished and can signal the caller to set up the status stage.
	if (!data_requested) {
		return false;
	}

	// Allocate a bounce buffer and pull data from the source into it.
	size_t max_packet = usb_device_info->ep0_max_packet;
	if (max_packet > data_requested) {
		max_packet = data_requested;
	}
	uint8_t packet_buffer[((max_packet + 3) / 4) * 4];
	size_t packet_size = 0;
	while (packet_size < max_packet) {
		size_t block_size = data_source->generate(data_source->opaque, &packet_buffer[packet_size], max_packet - packet_size);
		if (!block_size) {
			break;
		}
		packet_size += block_size;
	}

	// We’re here because either packet_size == max_packet or generate() returned zero indicating end of available data.

	// Enable the endpoint and lock in a transaction.
	OTG_FS_DIEPTSIZ0 = PKTCNT(1) // Transmit one packet.
		| XFRSIZ(packet_size); // Transmit packet_size bytes in this transaction (this may be zero).
	OTG_FS_DIEPCTL0 |=
		EPENA // Enable endpoint.
		| CNAK; // Stop NAKing all transactions.

	// Push the data into the FIFO.
	// We are guaranteed to have enough space because the endpoint 0 TX FIFO is large enough to hold max packet, and we only ever send one transaction at a time.
	size_t packet_words = (packet_size + 3) / 4;
	for (size_t i = 0; i < packet_words; ++i) {
		OTG_FS_FIFO[0][i] = ((const uint32_t *) packet_buffer)[i];
	}

	// Account for the data we just sent.
	data_requested -= packet_size;

	// There are five cases:
	// 1. We’re not at the end of the data stage. We just pushed a maximum-sized transaction, the host wants more data, and the data source can produce more data.
	//    Here, data_requested ≠ 0 and packet_size = ep0_max_packet.
	//    At some point we’ll get a transaction complete interrupt and come back here to push more data.
	//
	// 2. We’re at the end of the data stage. We just pushed a transaction (maximum-sized or not), and the host doesn’t want any more data.
	//    Here, data_requested = 0 and packet_size ≠ 0.
	//    At some point we’ll get a transaction complete interrupt and, because data_requested = 0, return false, signalling the transition to the status stage.
	//    No more packets will be generated, even if packet_size = ep0_max_packet this time, which is correct, because after wLength bytes, the host knows the data stage is over.
	//
	// 3. We’re at the end of the data stage. The data source ran out of data so we pushed a short packet, but the host would have wanted more data.
	//    Here, data_requested ≠ 0 and 0 < packet_size < ep0_max_packet.
	//    There is no need for a ZLP because the host recognizes the end of the data stage by the fact that packet_size ≠ ep0_max_packet.
	//    Consequently, at the next transaction complete interrupt, when the handler calls push_transaction() again, we must arrange to return false to signal the transition to the status stage.
	//    Do that by setting data_requested = 0 now.
	//
	// 4. We’re at the end of the data stage. The data source had no data at all so we pushed a zero-length packet, but the host would have wanted more data.
	//    Here, data_requested ≠ 0 and packet_size = 0.
	//    The ZLP we just sent indicates to the host the end of the data stage.
	//    Consequently, at the next transaction complete interrupt, when the handler calls push_transaction() again, we must arrange to return false to signal the transition to the status stage.
	//    Do that by setting data_requested = 0 now.
	//
	// 5. We’re at the end of the data stage. We just pushed a maximum-sized transaction, the data source has nothing left (though we don’t know that yet), but the host wants more data.
	//    We don’t actually have any way of knowing that this is our situation without calling the generator again.
	//    We don’t want to call the generator again because then we’d have to store its output somewhere.
	//    Instead, we just do nothing; on the next call to push_transaction(), we will fall into case 4 and generate a ZLP.
	//    This is the correct behaviour: after sending a maximum-sized packet and wanting to signal end-of-data before the host expects it, a ZLP must be sent.
	if (data_requested != 0 && packet_size != usb_device_info->ep0_max_packet) {
		data_requested = 0;
	}

	return true;
}

static void handle_setup_stage_done(void) {
	// A small buffer for staging miscellaneous bits and pieces of data for transmission, plus a source to read from it
	static uint8_t stash_buffer[2];
	static usb_ep0_memory_source_t stash_buffer_source;

	// Flush the endpoint 0 transmit FIFO.
	while (!(OTG_FS_GRSTCTL & AHBIDL));
	OTG_FS_GRSTCTL = GRSTCTL_TXFNUM(0) | TXFFLSH;
	while (OTG_FS_GRSTCTL & TXFFLSH);

	// Parse the packet.
	uint8_t request_type = setup_packet[0];
	uint8_t request = setup_packet[1];
	uint16_t value = setup_packet[2] | (((uint16_t) setup_packet[3]) << 8);
	uint16_t index = setup_packet[4] | (((uint16_t) setup_packet[5]) << 8);
	data_requested = setup_packet[6] | (((uint16_t) setup_packet[7]) << 8);

	// If length is zero, direction is ignored (mask it out for canonicalization).
	if (!data_requested) {
		request_type &= 0x7F;
	}

	// First try letting the application handle the request.
	bool application_handled = false, ok;
	data_source = 0;
	data_sink = 0;
	out_complete_callback = 0;
	if (data_requested) {
		if (request_type & 0x80) {
			if (!application_handled && global_callbacks->on_in_request) {
				application_handled = global_callbacks->on_in_request(request_type, request, value, index, data_requested, &data_source);
			}
			if (!application_handled && current_configuration_callbacks && current_configuration_callbacks->on_in_request) {
				application_handled = current_configuration_callbacks->on_in_request(request_type, request, value, index, data_requested, &data_source);
			}
			ok = !!data_source;
		} else {
			void *pdest = 0;
			if (!application_handled && current_configuration_callbacks && current_configuration_callbacks->on_out_request) {
				application_handled = current_configuration_callbacks->on_out_request(request_type, request, value, index, data_requested, &pdest, &out_complete_callback);
			}
			if (!application_handled && global_callbacks->on_out_request) {
				application_handled = global_callbacks->on_out_request(request_type, request, value, index, data_requested, &pdest, &out_complete_callback);
			}
			data_sink = pdest;
			ok = !!pdest;
		}
	} else {
		if (!application_handled && current_configuration_callbacks && current_configuration_callbacks->on_zero_request) {
			application_handled = current_configuration_callbacks->on_zero_request(request_type, request, value, index, &ok);
		}
		if (!application_handled && global_callbacks->on_zero_request) {
			application_handled = global_callbacks->on_zero_request(request_type, request, value, index, &ok);
		}
	}
	if (!application_handled) {
		ok = false;
		data_source = 0;
		data_sink = 0;
		out_complete_callback = 0;
	}

	// If the application didn’t handle the packet, examine it and try to find out what to do with it.
	if (!application_handled) {
		if (request_type == 0x80 && request == USB_STD_REQ_GET_STATUS && !value && !index && data_requested == 2) {
			// GET STATUS(DEVICE)
			// We do not support remote wakeup, so bit 1 is always set to zero.
			// We call the application to check whether we are currently bus-powered or self-powered.
			stash_buffer[0] = global_callbacks->on_check_self_powered() ? 0x01 : 0x00;
			stash_buffer[1] = 0x00;
			data_source = usb_ep0_memory_source_init(&stash_buffer_source, stash_buffer, 2);
			ok = true;
		} else if (request_type == 0x81 && request == USB_STD_REQ_GET_STATUS && !value && data_requested == 2) {
			// GET STATUS(INTERFACE)
			if (current_configuration_callbacks && (index & 0x00FF) < current_configuration_callbacks->interfaces) {
				stash_buffer[0] = 0x00;
				stash_buffer[1] = 0x00;
				data_source = usb_ep0_memory_source_init(&stash_buffer_source, stash_buffer, 2);
				ok = true;
			}
		} else if (request_type == 0x82 && request == USB_STD_REQ_GET_STATUS && !value && data_requested == 2) {
			// GET STATUS(ENDPOINT)
			uint8_t endpoint_direction = index & 0x0080;
			uint8_t endpoint_number = index & 0x000F;
			uint8_t max_endpoint;
			if (current_configuration_callbacks) {
				max_endpoint = endpoint_direction ? current_configuration_callbacks->in_endpoints : current_configuration_callbacks->out_endpoints;
			} else {
				max_endpoint = 0;
			}
			if (endpoint_number <= max_endpoint) {
				if (endpoint_number) {
					stash_buffer[0] = (endpoint_direction ? endpoints_callbacks->in_cbs : endpoints_callbacks->out_cbs)[endpoint_number - 1].is_halted() ? 0x01 : 0x00;
				} else {
					stash_buffer[0] = 0x00;
				}
				stash_buffer[1] = 0x00;
				data_source = usb_ep0_memory_source_init(&stash_buffer_source, stash_buffer, 2);
				ok = true;
			}
		} else if (request_type == 0x02 && request == USB_STD_REQ_CLEAR_FEATURE && !data_requested) {
			// CLEAR FEATURE(ENDPOINT)
			uint8_t endpoint_direction = index & 0x0080;
			uint8_t endpoint_number = index & 0x000F;
			uint8_t max_endpoint;
			if (current_configuration_callbacks) {
				max_endpoint = endpoint_direction ? current_configuration_callbacks->in_endpoints : current_configuration_callbacks->out_endpoints;
			} else {
				max_endpoint = 0;
			}
			if (endpoint_number <= max_endpoint) {
				if (value == 0) {
					// ENDPOINT_HALT
					if (endpoint_number) {
						ok = (endpoint_direction ? endpoints_callbacks->in_cbs : endpoints_callbacks->out_cbs)[endpoint_number - 1].on_clear_halt();
					} else {
						ok = true;
					}
				}
			}
		} else if (request_type == 0x02 && request == USB_STD_REQ_SET_FEATURE && !data_requested) {
			GPIOB_BSRR = GPIO_BS(14);
			// SET FEATURE(ENDPOINT)
			uint8_t endpoint_direction = index & 0x0080;
			uint8_t endpoint_number = index & 0x000F;
			uint8_t max_endpoint;
			if (current_configuration_callbacks) {
				max_endpoint = endpoint_direction ? current_configuration_callbacks->in_endpoints : current_configuration_callbacks->out_endpoints;
			} else {
				max_endpoint = 0;
			}
			if (endpoint_number <= max_endpoint) {
				if (value == 0) {
					// ENDPOINT_HALT
					if (endpoint_number) {
						(endpoint_direction ? endpoints_callbacks->in_cbs : endpoints_callbacks->out_cbs)[endpoint_number - 1].on_halt();
					}
					ok = true;
				}
			}
		} else if (request_type == 0x00 && request == USB_STD_REQ_SET_ADDRESS && !index && !data_requested) {
			// SET ADDRESS(DEVICE)
			uint8_t address = setup_packet[2];
			// SET_ADDRESS is only legal in the Default and Addressed states and the address must be 127 or less.
			if (!current_configuration && address <= 127) {
				// Lock in the address; the hardware knows to stay on address zero until the status stage is complete and, in fact, *FAILS* if the address is locked in later!
				OTG_FS_DCFG = (OTG_FS_DCFG & ~DCFG_DAD_MSK) | DCFG_DAD(address);
				ok = true;
			}
		} else if (request_type == 0x80 && request == USB_STD_REQ_GET_DESCRIPTOR) {
			// GET DESCRIPTOR
			data_source = global_callbacks->on_descriptor_request(setup_packet[3], setup_packet[2], index);
			ok = !!data_source;
		} else if (request_type == 0x80 && request == USB_STD_REQ_GET_CONFIGURATION && !value && !index && data_requested == 1) {
			// GET CONFIGURATION(DEVICE)
			if (DCFG_DAD_X(OTG_FS_DCFG)) {
				stash_buffer[0] = current_configuration;
				data_source = usb_ep0_memory_source_init(&stash_buffer_source, stash_buffer, 1);
				ok = true;
			}
		} else if (request_type == 0x00 && request == USB_STD_REQ_SET_CONFIGURATION && !index && !data_requested) {
			// SET CONFIGURATION(DEVICE)
			// This is only legal in the Addressed and Configured states.
			if (DCFG_DAD_X(OTG_FS_DCFG)) {
				const usb_ep0_configuration_callbacks_t *new_cbs = 0;
				if (value) {
					// Check if the specified configuration exists and is currently available.
					ok = false;
					for (const usb_ep0_configuration_callbacks_t * const *i = configuration_callbacks; *i; ++i) {
						if ((*i)->configuration == value) {
							new_cbs = *i;
							if (new_cbs->can_enter) {
								ok = new_cbs->can_enter();
							} else {
								ok = true;
							}
						}
					}
				} else {
					ok = true;
				}

				// Only actually execute the transition between configurations if the new configuration is acceptable.
				if (ok) {
					if (current_configuration_callbacks && current_configuration_callbacks->on_exit) {
						current_configuration_callbacks->on_exit();
					}
					current_configuration = value;
					current_configuration_callbacks = new_cbs;
					if (new_cbs && new_cbs->on_enter) {
						new_cbs->on_enter();
					}
				}
			}
		}
	}

	if (!ok) {
		// On failure, just stall everything.
		OTG_FS_DIEPCTL0 |= DIEPCTL_STALL;
		OTG_FS_DOEPCTL0 |= DOEPCTL_STALL;
	} else if (request_type & 0x80) {
		// The next thing to set up is a data stage comprising IN data transactions.
		// Start the first transaction.
		push_transaction();

		// Enable OUT endpoint 0 for a status stage as it is automatically disabled as part of SETUP transaction processing.
		// We should enable this now rather than waiting for all transaction complete notifications for two reasons:
		// (1) During initial enumeration, Linux sends a GET_DESCRIPTOR(DEVICE) with wLength=64, but if EP0 max packet=8 then accepts only one 8-byte transaction before the status stage.
		// (2) In general, if the last IN transaction in the data stage suffers a lost ACK, the host will try to start the status stage while the device believes the data stage is still running; should this happen, the correct outcome is that the status stage starts (this being an IN data stage, the device shouldn’t care whether it actually knows the data was delivered or not).
		OTG_FS_DOEPTSIZ0 = STUPCNT(3) // Allow up to 3 back-to-back SETUP data packets.
			| PKTCNT(1) // Accept one packet.
			| XFRSIZ(0); // Accept zero bytes.
		OTG_FS_DOEPCTL0 |= EPENA; // Enable endpoint.
		OTG_FS_DOEPCTL0 |= CNAK; // Stop NAKing packets.
	} else if (data_requested) {
		// Enable OUT endpoint 0 for the data stage.
		// It is not necessary to enable IN endpoint 0 at this time because the host guarantees to send exactly as much data as it says it wishes to.
		// We can therefore safely wait until we have received that much data before moving into the status stage.
		// Thus, for now, leave IN endpoint 0 disabled.
		uint8_t packet_size = usb_device_info->ep0_max_packet;
		if (packet_size > data_requested) {
			packet_size = data_requested;
		}
		OTG_FS_DOEPTSIZ0 = STUPCNT(3) // Allow up to 3 back-to-back SETUP data packets.
			| PKTCNT(1) // Accept one packet.
			| XFRSIZ(packet_size); // Accept min{max-packet|data-requested} bytes.
		OTG_FS_DOEPCTL0 |= EPENA; // Enable endpoint.
		OTG_FS_DOEPCTL0 |= CNAK; // Stop NAKing packets.
	} else {
		// The next thing to set up is a status stage comprising an IN data transaction.
		OTG_FS_DIEPTSIZ0 = PKTCNT(1) // Transmit one packet.
			| XFRSIZ(0); // Transmit 0 bytes in the whole transfer.
		OTG_FS_DIEPCTL0 &= ~DIEPCTL_STALL; // Stop stalling transactions on this endpoint.
		OTG_FS_DIEPCTL0 |=
			EPENA // Enable endpoint.
			| CNAK; // Stop NAKing all transactions.
	}

	// Whatever happened, reset the setup counter in the OUT endpoint so we can receive additional SETUP transactions.
	OTG_FS_DOEPTSIZ0 |= STUPCNT(3); // Allow up to 3 back-to-back SETUP data packets.
}

void usb_ep0_handle_receive(uint32_t status_word) {
	static usb_gnak_request_t gnak_request = USB_GNAK_REQUEST_INIT;

	// See what happened.
	size_t bcnt = (status_word >> 4) & 0x7FF;
	uint8_t pktsts = (status_word >> 17) & 0xF;
	switch (pktsts) {
		case 0x2:
			// OUT data packet received
			// Copy out the packet.
			usb_copy_out_packet(data_sink, bcnt);

			if (data_sink) {
				// This is part of an OUT data stage. Handle it appropriately.
				data_sink += bcnt;
				data_requested -= bcnt;
				if (!data_requested) {
					// The data stage is finished and it’s time to do the status stage.
					bool ok;
					if (out_complete_callback) {
						ok = out_complete_callback();
					} else {
						ok = true;
					}
					if (ok) {
						// Set up an IN status stage.
						OTG_FS_DIEPTSIZ0 = PKTCNT(1) // Transmit one packet.
							| XFRSIZ(0); // Transmit 0 bytes in the whole transfer.
						OTG_FS_DIEPCTL0 &= ~DIEPCTL_STALL; // Stop stalling transactions on this endpoint.
						OTG_FS_DIEPCTL0 |=
							EPENA // Enable endpoint.
							| CNAK; // Stop NAKing all transactions.
					} else {
						// On failure, just stall everything.
						OTG_FS_DIEPCTL0 |= DIEPCTL_STALL;
						OTG_FS_DOEPCTL0 |= DOEPCTL_STALL;
					}
				} else {
					// There is more data to transfer.
					uint8_t packet_size = usb_device_info->ep0_max_packet;
					if (packet_size > data_requested) {
						packet_size = data_requested;
					}
					OTG_FS_DOEPTSIZ0 = STUPCNT(3) // Allow up to 3 back-to-back SETUP data packets.
						| PKTCNT(1) // Accept one packet.
						| XFRSIZ(packet_size); // Accept min{max-packet|data-requested} bytes.
					OTG_FS_DOEPCTL0 |= EPENA; // Enable endpoint.
					OTG_FS_DOEPCTL0 |= CNAK; // Stop NAKing packets.
				}
			} else {
				// This is part of an OUT status stage.
				// Nothing to do here.
			}
			break;

		case 0x3:
			// OUT transfer completed
			// Clean up the endpoint.
			OTG_FS_DOEPTSIZ0 |= STUPCNT(3); // Allow up to 3 back-to-back SETUP data packets.
			break;

		case 0x4:
			// SETUP stage completed
			usb_set_global_nak(&gnak_request, &handle_setup_stage_done);
			break;

		case 0x6:
			// SETUP data packet received
			// Copy into SETUP packet buffer.
			if (bcnt == sizeof(setup_packet)) {
				usb_copy_out_packet(setup_packet, sizeof(setup_packet));
			} else {
				usb_copy_out_packet(0, bcnt);
			}
			break;

	}
}

static void on_in_transaction_complete(void) {
	push_transaction();
}

static void on_in_endpoint_event(void) {
	// Find out what happened.
	uint32_t diepint = OTG_FS_DIEPINT0;
	if (diepint & XFRC) {
		OTG_FS_DIEPINT0 = XFRC;
		on_in_transaction_complete();
	}
}

void usb_ep0_init(void) {
	// We currently have no configuration.
	current_configuration = 0;

	// Register callbacks.
	usb_in_set_callback(0, &on_in_endpoint_event);

	// Set max packet size.
	switch (usb_device_info->ep0_max_packet) {
		case 8:
			OTG_FS_DIEPCTL0 = MPSIZ(0x3);
			break;
		case 16:
			OTG_FS_DIEPCTL0 = MPSIZ(0x2);
			break;
		case 32:
			OTG_FS_DIEPCTL0 = MPSIZ(0x1);
			break;
		case 64:
			OTG_FS_DIEPCTL0 = MPSIZ(0x0);
			break;
	}

	// Enable interrupts to this endpoint.
	OTG_FS_DAINTMSK |= IEPM(1 << 0); // Take an interrupt on IN endpoint 0 event.

	// Enable OUT endpoint 0 to receive data.
	OTG_FS_DOEPTSIZ0 = STUPCNT(3) // Allow up to 3 back-to-back SETUP data packets.
		| PKTCNT(0) // No non-SETUP packets should be issued.
		| XFRSIZ(0); // No non-SETUP transfer is occurring.
}

void usb_ep0_deinit(void) {
	// If we are in a configuration, we exit it in the process of deinitializing.
	if (current_configuration) {
		if (current_configuration_callbacks && current_configuration_callbacks->on_exit) {
			current_configuration_callbacks->on_exit();
		}
		current_configuration = 0;
		current_configuration_callbacks = 0;
	}

	// Disable OUT endpoint 0 from receiving data, including SETUP packets.
	OTG_FS_DOEPTSIZ0 = STUPCNT(0) | PKTCNT(0) | XFRSIZ(0);

	// Disable interrupts to this endpoint.
	OTG_FS_DAINTMSK &= ~IEPM(1 << 0);

	// Unregister callback.
	usb_in_set_callback(0, 0);
}

void usb_ep0_set_global_callbacks(const usb_ep0_global_callbacks_t *callbacks) {
	global_callbacks = callbacks;
}

void usb_ep0_set_configuration_callbacks(const usb_ep0_configuration_callbacks_t * const *configurations) {
	configuration_callbacks = configurations;
}

void usb_ep0_set_endpoints_callbacks(const usb_ep0_endpoints_callbacks_t *cbs) {
	endpoints_callbacks = cbs;
}

uint8_t usb_ep0_get_configuration(void) {
	return current_configuration;
}

