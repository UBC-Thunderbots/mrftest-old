#include <usb_ep0.h>
#include <assert.h>
#include <minmax.h>
#include <registers/otg_fs.h>
#include <stdlib.h>
#include <unused.h>
#include <usb_fifo.h>
#include <usb_ll.h>

static size_t ep0_max_packet = 0;
static const usb_ep0_cbs_t *cb_stack[USB_EP0_CBS_STACK_SIZE];

static usb_ep0_setup_packet_t setup_packet;
static size_t data_requested = 0;
static usb_ep0_postdata_cb_t postdata = 0;
static usb_ep0_poststatus_cb_t poststatus = 0;
static usb_ep0_source_t *data_source = 0;
static uint8_t *data_sink = 0;
static bool in_is_status = false;

static bool push_in_transaction(void) {
	// If the host isn’t expecting us to send any data, we’re finished and can signal the caller to set up the status stage.
	if (!data_requested) {
		return false;
	}

	// Allocate a bounce buffer and pull data from the source into it.
	size_t max_packet = ep0_max_packet;
	if (max_packet > data_requested) {
		max_packet = data_requested;
	}
	uint32_t packet_buffer[(max_packet + 3) / 4];
	size_t packet_size = 0;
	uint8_t *packet_write_ptr = (uint8_t *) packet_buffer;
	while (packet_size < max_packet) {
		size_t block_size = data_source->generate(data_source->opaque, &packet_write_ptr[packet_size], max_packet - packet_size);
		if (!block_size) {
			break;
		}
		packet_size += block_size;
	}

	// We’re here because either packet_size == max_packet or generate() returned zero indicating end of available data.

	// Enable the endpoint and lock in a transaction.
	{
		OTG_FS_DIEPTSIZ0_t tmp = {
			.PKTCNT = 1, // Transmit one packet.
			.XFRSIZ = packet_size, // Transmit packet_size bytes in this transaction (this may be zero).
		};
		OTG_FS_DIEPTSIZ0 = tmp;
	}
	{
		OTG_FS_DIEPCTL0_t tmp = OTG_FS_DIEPCTL0;
		tmp.EPENA = 1; // Enable endpoint.
		tmp.CNAK = 1; // Stop NAKing all transactions.
		OTG_FS_DIEPCTL0 = tmp;
	}

	// Push the data into the FIFO.
	// We are guaranteed to have enough space because the endpoint 0 TX FIFO is large enough to hold max packet, and we only ever send one transaction at a time.
	size_t packet_words = (packet_size + 3) / 4;
	for (size_t i = 0; i < packet_words; ++i) {
		OTG_FS_FIFO[0][0] = packet_buffer[i];
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
	//    Consequently, at the next transaction complete interrupt, when the handler calls push_in_transaction() again, we must arrange to return false to signal the transition to the status stage.
	//    Do that by setting data_requested = 0 now.
	//
	// 4. We’re at the end of the data stage. The data source had no data at all so we pushed a zero-length packet, but the host would have wanted more data.
	//    Here, data_requested ≠ 0 and packet_size = 0.
	//    The ZLP we just sent indicates to the host the end of the data stage.
	//    Consequently, at the next transaction complete interrupt, when the handler calls push_in_transaction() again, we must arrange to return false to signal the transition to the status stage.
	//    Do that by setting data_requested = 0 now.
	//
	// 5. We’re at the end of the data stage. We just pushed a maximum-sized transaction, the data source has nothing left (though we don’t know that yet), but the host wants more data.
	//    We don’t actually have any way of knowing that this is our situation without calling the generator again.
	//    We don’t want to call the generator again because then we’d have to store its output somewhere.
	//    Instead, we just do nothing; on the next call to push_in_transaction(), we will fall into case 4 and generate a ZLP.
	//    This is the correct behaviour: after sending a maximum-sized packet and wanting to signal end-of-data before the host expects it, a ZLP must be sent.
	if (data_requested != 0 && packet_size != ep0_max_packet) {
		data_requested = 0;
	}

	return true;
}

static void handle_setup_stage_done_no_data(void) {
	// Go through the callbacks in the stack and find out the final disposition of the request.
	usb_ep0_disposition_t disp = USB_EP0_DISPOSITION_NONE;
	for (unsigned int i = USB_EP0_CBS_STACK_SIZE - 1; i < USB_EP0_CBS_STACK_SIZE && disp == USB_EP0_DISPOSITION_NONE; --i) {
		if (cb_stack[i] && cb_stack[i]->on_zero_request) {
			disp = cb_stack[i]->on_zero_request(&setup_packet, &poststatus);
		}
	}

	// If no callback took responsibility, reject the request.
	if (disp == USB_EP0_DISPOSITION_NONE) {
		disp = USB_EP0_DISPOSITION_REJECT;
	}

	// Set up the status stage on the IN endpoint.
	if (disp == USB_EP0_DISPOSITION_ACCEPT) {
		// Request was accepted, so set a zero-length packet.
		in_is_status = true;
		{
			OTG_FS_DIEPTSIZ0_t tmp = {
				.PKTCNT = 1, // Transmit one packet.
				.XFRSIZ = 0, // Transmit 0 bytes in the whole transfer.
			};
			OTG_FS_DIEPTSIZ0 = tmp;
		}
		{
			OTG_FS_DIEPCTL0_t tmp = OTG_FS_DIEPCTL0;
			tmp.STALL = 0; // Stop stalling transactions on this endpoint.
			tmp.EPENA = 1; // Enable endpoint.
			tmp.CNAK = 1; // Stop NAKing all transactions.
			OTG_FS_DIEPCTL0 = tmp;
		}
	} else {
		// Request was rejected, so stall the IN endpoint.
		OTG_FS_DIEPCTL0.STALL = 1;
	}
}

static void handle_setup_stage_done_in_data(void) {
	// Go through the callbacks in the stack and find out the final disposition of the request.
	usb_ep0_disposition_t disp = USB_EP0_DISPOSITION_NONE;
	for (unsigned int i = USB_EP0_CBS_STACK_SIZE - 1; i < USB_EP0_CBS_STACK_SIZE && disp == USB_EP0_DISPOSITION_NONE; --i) {
		if (cb_stack[i] && cb_stack[i]->on_in_request) {
			disp = cb_stack[i]->on_in_request(&setup_packet, &data_source, &poststatus);
		}
	}

	// If no callback took responsibility, reject the request.
	if (disp == USB_EP0_DISPOSITION_NONE) {
		disp = USB_EP0_DISPOSITION_REJECT;
	}

	// Set up the data stage on the IN endpoint.
	if (disp == USB_EP0_DISPOSITION_ACCEPT) {
		// Request was accepted, so push a transaction.
		push_in_transaction();

		// Enable OUT endpoint 0 for a status stage as it is automatically disabled as part of SETUP transaction processing.
		// We should enable this now rather than waiting for all transaction complete notifications on the IN endpoint for two reasons:
		// (1) During initial enumeration, Linux sends a GET_DESCRIPTOR(DEVICE) with wLength=64, but if EP0 max packet=8 then accepts only one 8-byte transaction before the status stage.
		// (2) In general, if the last IN transaction in the data stage suffers a lost ACK, the host will try to start the status stage while the device believes the data stage is still running; should this happen, the correct outcome is that the status stage starts (this being an IN data stage, the device shouldn’t care whether it actually knows the data was delivered or not).
		{
			OTG_FS_DOEPTSIZ0_t tmp = {
				.PKTCNT = 1, // Accept one packet.
				.XFRSIZ = 0, // Accept zero bytes.
			};
			OTG_FS_DOEPTSIZ0 = tmp;
		}
		{
			OTG_FS_DOEPCTL0_t tmp = OTG_FS_DOEPCTL0;
			tmp.STALL = 0; // Stop stalling.
			tmp.EPENA = 1; // Enable endpoint.
			tmp.CNAK = 1; // Stop NAKing packets.
			OTG_FS_DOEPCTL0 = tmp;
		}
	} else {
		// Request was rejected, so stall the IN endpoint (for the data stage) and the OUT endpoint (for the status stage).
		OTG_FS_DIEPCTL0.STALL = 1;
		OTG_FS_DOEPCTL0.STALL = 1;
	}
}

static void handle_setup_stage_done_out_data(void) {
	// Go through the callbacks in the stack and find out the final disposition of the request.
	usb_ep0_disposition_t disp = USB_EP0_DISPOSITION_NONE;
	for (unsigned int i = USB_EP0_CBS_STACK_SIZE - 1; i < USB_EP0_CBS_STACK_SIZE && disp == USB_EP0_DISPOSITION_NONE; --i) {
		if (cb_stack[i] && cb_stack[i]->on_out_request) {
			void *sink;
			disp = cb_stack[i]->on_out_request(&setup_packet, &sink, &postdata, &poststatus);
			data_sink = sink;
		}
	}

	// If no callback took responsibility, reject the request.
	if (disp == USB_EP0_DISPOSITION_NONE) {
		disp = USB_EP0_DISPOSITION_REJECT;
	}

	// Set up the data stage on the IN endpoint.
	if (disp == USB_EP0_DISPOSITION_ACCEPT) {
		// Enable OUT endpoint 0 for the data stage.
		// It is not necessary to enable IN endpoint 0 at this time because the host guarantees to send exactly as much data as it says it wishes to.
		// We can therefore safely wait until we have received that much data before moving into the status stage.
		// Thus, for now, leave IN endpoint 0 disabled.
		// This also allows the postdata callback to reject the transfer by stalling the status stage.
		{
			OTG_FS_DOEPTSIZ0_t tmp = {
				.PKTCNT = 1, // Accept one packet.
				.XFRSIZ = MIN(ep0_max_packet, data_requested), // Accept min{max-packet|data-requested} bytes.
			};
			OTG_FS_DOEPTSIZ0 = tmp;
		}
		{
			OTG_FS_DOEPCTL0_t tmp = OTG_FS_DOEPCTL0;
			tmp.STALL = 0; // Stop stalling the endpoint.
			tmp.EPENA = 1; // Enable endpoint.
			tmp.CNAK = 1; // Stop NAKing packets.
			OTG_FS_DOEPCTL0 = tmp;
		}
	} else {
		// Request was rejected, so stall the OUT endpoint (for the data stage) and the IN endpoint (for the status stage).
		OTG_FS_DOEPCTL0.STALL = 1;
		OTG_FS_DIEPCTL0.STALL = 1;
	}
}

static void handle_setup_stage_done(void) {
	// Flush the endpoint 0 transmit FIFO.
	usb_fifo_flush(0);

	// Parse the packet and clear variables.
	data_requested = setup_packet.length;
	postdata = 0;
	poststatus = 0;
	data_source = 0;
	data_sink = 0;
	in_is_status = false;

	// If length is zero, direction is ignored (mask it out for canonicalization).
	if (!data_requested) {
		setup_packet.request_type &= 0x7F;
	}

	// Now handle based on the type of data stage.
	if (!data_requested) {
		handle_setup_stage_done_no_data();
	} else if (setup_packet.request_type & 0x80) {
		handle_setup_stage_done_in_data();
	} else {
		handle_setup_stage_done_out_data();
	}
}

static void handle_receive_pattern(unsigned int UNUSED(ep), OTG_FS_GRXSTSR_device_t status_word) {
	static usb_ll_gnak_req_t gnak_request = USB_LL_GNAK_REQ_INIT;

	// See what happened.
	switch (status_word.PKTSTS) {
		case 0x2:
			// OUT data packet received.
			// This might be the status stage of an IN or zero-data request (in which case it will be zero length).
			// It might also be a data stage of an OUT request.
			// Copy out the packet into the data sink; this will do nothing if zero length.
			for (size_t i = 0; i < status_word.BCNT; i += 4) {
				uint32_t word = OTG_FS_FIFO[0][0];
				data_sink[i + 0] = word;
				data_sink[i + 1] = word >> 8;
				data_sink[i + 2] = word >> 16;
				data_sink[i + 3] = word >> 24;
			}
			data_sink += status_word.BCNT;
			data_requested -= status_word.BCNT;
			break;

		case 0x3:
			// OUT transfer completed.
			if (data_sink) {
				// This is part of an OUT data stage. Handle it appropriately.
				if (!data_requested) {
					// The data stage is finished and it’s time to do the status stage.
					// First invoke the postdata callback and see whether to accept the request.
					bool ok;
					if (postdata) {
						ok = postdata();
					} else {
						ok = true;
					}

					// Set up the status stage on the IN endpoint.
					if (ok) {
						// Request was accepted, so set a zero-length packet.
						in_is_status = true;
						{
							OTG_FS_DIEPTSIZ0_t tmp = {
								.PKTCNT = 1, // Transmit one packet.
								.XFRSIZ = 0, // Transmit 0 bytes in the whole transfer.
							};
							OTG_FS_DIEPTSIZ0 = tmp;
						}
						{
							OTG_FS_DIEPCTL0_t tmp = OTG_FS_DIEPCTL0;
							tmp.STALL = 0; // Stop stalling transactions on this endpoint.
							tmp.EPENA = 1;// Enable endpoint.
							tmp.CNAK = 1; // Stop NAKing all transactions.
							OTG_FS_DIEPCTL0 = tmp;
						}
					} else {
						// Request was rejected, so stall the IN endpoint.
						OTG_FS_DIEPCTL0.STALL = 1;
					}
				} else {
					// There is more data to transfer.
					{
						OTG_FS_DOEPTSIZ0_t tmp = {
							.PKTCNT = 1, // Accept one packet.
							.XFRSIZ = MIN(ep0_max_packet, data_requested), // Accept min{max-packet|data-requested} bytes.
						};
						OTG_FS_DOEPTSIZ0 = tmp;
					}
					{
						OTG_FS_DOEPCTL0_t tmp = OTG_FS_DOEPCTL0;
						tmp.EPENA = 1;// Enable endpoint.
						tmp.CNAK = 1; // Stop NAKing packets.
						OTG_FS_DOEPCTL0 = tmp;
					}
				}
			} else {
				// This is part of an OUT status stage.
				// Invoke the poststatus callback.
				if (poststatus) {
					poststatus();
					poststatus = 0;
				}
			}
			break;

		case 0x4:
			// SETUP stage completed.
			usb_ll_set_gnak(&gnak_request, &handle_setup_stage_done);
			break;

		case 0x6:
			// SETUP data packet received.
			// Copy into SETUP packet buffer.
			if (status_word.BCNT == 8) {
				uint32_t word = OTG_FS_FIFO[0][0];
				setup_packet.request_type = word;
				setup_packet.request = word >> 8;
				setup_packet.value = word >> 16;
				word = OTG_FS_FIFO[0][0];
				setup_packet.index = word;
				setup_packet.length = word >> 16;
			} else {
				for (size_t i = 0; i < status_word.BCNT; i += 4) {
					(void) OTG_FS_FIFO[0][0];
				}
			}
			break;
	}
}

static void on_in_transaction_complete(void) {
	if (in_is_status) {
		// This was an IN status stage.
		// Invoke the poststatus callback if registered.
		if (poststatus) {
			poststatus();
			poststatus = 0;
			in_is_status = false;
		}
	} else {
		// This was part of an IN data stage.
		// Push another transaction.
		// There might not be any more transactions to push, in which case we actually don’t have to do anything.
		// This is because, for an IN data stage, we have already set up the OUT status stage earlier.
		push_in_transaction();
	}
}

static void on_in_endpoint_event(unsigned int UNUSED(ep)) {
	// Find out what happened.
	OTG_FS_DIEPINTx_t diepint = OTG_FS_DIEP[0].DIEPINT;
	if (diepint.XFRC) {
		{
			OTG_FS_DIEPINTx_t tmp = { .XFRC = 1 };
			OTG_FS_DIEP[0].DIEPINT = tmp;
		}
		on_in_transaction_complete();
	}
}

void usb_ep0_init(size_t max_packet) {
	// Sanity check.
	assert(max_packet == 8 || max_packet == 16 || max_packet == 32 || max_packet == 64);

	// Initialize variables.
	ep0_max_packet = max_packet;
	for (unsigned int i = 0; i < USB_EP0_CBS_STACK_SIZE; ++i) {
		cb_stack[i] = 0;
	}
	data_requested = 0;
	postdata = 0;
	poststatus = 0;
	data_source = 0;
	data_sink = 0;
	in_is_status = false;

	// Register callbacks.
	usb_ll_in_set_cb(0, &on_in_endpoint_event);
	usb_ll_out_set_cb(0, &handle_receive_pattern);

	// Set max packet size.
	{
		OTG_FS_DIEPCTL0_t tmp = { 0 };
		switch (ep0_max_packet) {
			case 8:
				tmp.MPSIZ = 0x3;
				break;
			case 16:
				tmp.MPSIZ = 0x2;
				break;
			case 32:
				tmp.MPSIZ = 0x1;
				break;
			case 64:
				tmp.MPSIZ = 0x0;
				break;
		}
		OTG_FS_DIEPCTL0 = tmp;
	}

	// Enable interrupts to this endpoint.
	OTG_FS_DAINTMSK.IEPM |= 1 << 0; // Take an interrupt on IN endpoint 0 event.
}

void usb_ep0_deinit(void) {
	// Disable both endpoints from receiving data packets.
	if (OTG_FS_DOEPCTL0.EPENA) {
		OTG_FS_DOEPCTL0.EPDIS = 1;
		while (OTG_FS_DOEPCTL0.EPENA);
	}
	if (OTG_FS_DIEPCTL0.EPENA) {
		OTG_FS_DIEPCTL0.EPDIS = 1;
		while (OTG_FS_DIEPCTL0.EPENA);
	}

	// Disable OUT endpoint 0 from receiving SETUP packets.
	{
		OTG_FS_DOEPTSIZ0_t tmp = { 0 };
		OTG_FS_DOEPTSIZ0 = tmp;
	}

	// Disable interrupts to this endpoint.
	OTG_FS_DAINTMSK.IEPM &= ~(1 << 0);

	// Unregister callbacks.
	usb_ll_in_set_cb(0, 0);
	usb_ll_out_set_cb(0, 0);
}

void usb_ep0_cbs_push(const usb_ep0_cbs_t *cbs) {
	usb_ep0_cbs_remove(cbs);
	for (unsigned int i = 0; i < USB_EP0_CBS_STACK_SIZE; ++i) {
		if (!cb_stack[i]) {
			cb_stack[i] = cbs;
			return;
		}
	}
	abort();
}

void usb_ep0_cbs_remove(const usb_ep0_cbs_t *cbs) {
	unsigned int r = 0, w = 0;
	while (r < USB_EP0_CBS_STACK_SIZE) {
		if (cb_stack[r] != cbs) {
			cb_stack[w++] = cb_stack[r];
		}
		++r;
	}
	while (w < USB_EP0_CBS_STACK_SIZE) {
		cb_stack[w++] = 0;
	}
}

