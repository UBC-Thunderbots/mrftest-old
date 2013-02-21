#include "promiscuous.h"
#include "config.h"
#include "constants.h"
#include "mrf.h"
#include "perconfig.h"
#include <exti.h>
#include <rcc.h>
#include <registers.h>
#include <sleep.h>
#include <stddef.h>
#include <stdint.h>
#include <unused.h>
#include <usb_bi_in.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_fifo.h>

const uint8_t PROMISCUOUS_CONFIGURATION_DESCRIPTOR[] = {
	9, // bLength
	2, // bDescriptorType
	25, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	3, // bConfigurationValue
	STRING_INDEX_CONFIG3, // iConfiguration
	0x80, // bmAttributes
	150, // bMaxPower

	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	1, // bNumEndpoints
	0xFF, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	5, // bDescriptorType
	0x81, // bEndpointAddress
	0x02, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	0, // bInterval
};

static uint16_t promisc_flags;
static promisc_packet_t *first_free_packet, *first_captured_packet, *last_captured_packet;
static bool packet_dropped;

static void push_data(void) {
	// If the received message endpoint already has data queued or is halted, don’t try to send more data.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can push more messages.
	usb_bi_in_state_t state = usb_bi_in_get_state(1);
	if (state == USB_BI_IN_STATE_ACTIVE || state == USB_BI_IN_STATE_HALTED) {
		return;
	}

	// If there are no received messages waiting to be sent, do nothing.
	// We will get back here later when an MRF interrupt leads to a received message being queued and we can then push the message.
	if (!first_captured_packet) {
		return;
	}

	// Pop a packet from the queue.
	promisc_packet_t *packet = first_captured_packet;
	first_captured_packet = packet->next;
	if (!first_captured_packet) {
		last_captured_packet = 0;
	}
	packet->next = 0;

	// Start a transfer.
	// Received message transfers are variable length up to a known maximum size.
	usb_bi_in_start_transfer(1, packet->length, sizeof(packet->data), &push_data, 0);

	// Push the data for this transfer.
	usb_bi_in_push(1, packet->data, packet->length);

	// Push this consumed packet buffer into the free list.
	packet->next = first_free_packet;
	first_free_packet = packet;
}

static void exti12_interrupt_vector(void) {
	// Clear the interrupt.
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	while (GPIOC_IDR & (1 << 12) /* PC12 */) {
		// Check outstanding interrupts.
		uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
		if (intstat & (1 << 3)) {
			// RXIF = 1; packet received.
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
			uint8_t rxfifo_frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO); // Need to read this here even if no packet buffer because this read also re-enables the receiver

			static const size_t BUFFER_OVERHEAD = 1 /* Flags */ + 1 /* Channel */ + 1 /* LQI */ + 1 /* RSSI */;

			// Sanitize the frame length to avoid buffer overruns.
			if (rxfifo_frame_length > sizeof(first_free_packet->data) - BUFFER_OVERHEAD) {
				rxfifo_frame_length = sizeof(first_free_packet->data) - BUFFER_OVERHEAD;
			}

			// Allocate a packet.
			promisc_packet_t *packet = first_free_packet;
			if (packet) {
#warning proper packet filtering when radio filters do not match capture flags exactly
				first_free_packet = packet->next;
				packet->next = 0;
				packet->length = 1 /* Flags */ + 1 /* Channel */ + rxfifo_frame_length /* MAC header + data + FCS */ + 1 /* LQI */ + 1 /* RSSI */;
				packet->data[0] = packet_dropped ? 0x01 : 0x00;
				packet_dropped = false;
				packet->data[1] = config.channel;
				for (size_t i = 0; i < rxfifo_frame_length + 2U /* LQI + RSSI */; ++i) {
					packet->data[2 + i] = mrf_read_long(MRF_REG_LONG_RXFIFO + i + 1);
				}
				if (last_captured_packet) {
					last_captured_packet->next = packet;
					last_captured_packet = packet;
				} else {
					first_captured_packet = last_captured_packet = packet;
				}
				push_data();
			} else {
				packet_dropped = true;
			}
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
			// Toggle LED 3 to show reception.
			GPIOB_ODR ^= 1 << 14;
		}
	}
}

static void handle_ep1_clear_halt(unsigned int UNUSED(ep)) {
	// Free all the queued packets.
	while (first_captured_packet) {
		promisc_packet_t *packet = first_captured_packet;
		first_captured_packet = packet->next;
		packet->next = first_free_packet;
		first_free_packet = packet;
	}
	last_captured_packet = 0;
}

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_SET_PROMISCUOUS_FLAGS) {
		if (!pkt->index) {
			// Sanity check: flags 8 through 15 are reserved and must be zero.
			if (pkt->value & 0xFF00) {
				return USB_EP0_DISPOSITION_REJECT;
			}
			// Check if the application actually wants *ANY* packets.
			if (pkt->value & 0xF0) {
				// Sanity check: if flag 0 (acknowledge) is set and one of bits 4 through 7 (accept some type of frame) is set…
				if ((pkt->value & 0x0001) && (pkt->value & 0xF0)) {
					// … either exactly one or all three of bits 4 through 6 must be set to 1…
					if ((pkt->value & 0x70) != 0x10 && (pkt->value & 0x70) != 0x20 && (pkt->value & 0x70) != 0x40 && (pkt->value & 0x70) != 0x70) {
						return USB_EP0_DISPOSITION_REJECT;
					}
					// … either none or all of bits 1, 2, and 7 must be set to 1…
					if ((pkt->value & 0x86) != 0x00 && (pkt->value & 0x86) != 0x86) {
						return USB_EP0_DISPOSITION_REJECT;
					}
					// … and if bit 7 is set to 1, bits 4 through 6 must also all be set to 1
					if ((pkt->value & 0x80) && (pkt->value & 0x70) != 0x70) {
						return USB_EP0_DISPOSITION_REJECT;
					}
				}
				// This set of flags is acceptable; save.
				promisc_flags = pkt->value;
				// Disable all packet reception.
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04);
				// Install the new flags.
				mrf_write_short(MRF_REG_SHORT_RXMCR,
						((pkt->value & (1 << 0)) ? 0 : (1 << 5))
						| ((pkt->value & (1 << 3)) ? (1 << 1) : 0)
						| ((pkt->value & 0x86) ? (1 << 0) : 0));
				if ((pkt->value & 0xF0) == 0x10) {
					mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x64);
				} else if ((pkt->value & 0xF0) == 0x20) {
					mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x68);
				} else if ((pkt->value & 0xF0) == 0x40) {
					mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x62);
				} else {
					mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x60);
				}
				// Set analogue path appropriately based on whether ACKs are being generated and whether any packets are desired.
				if (pkt->value & 0x01) {
					mrf_analogue_txrx();
				} else {
					mrf_analogue_rx();
				}
				// Re-enable packet reception.
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00);
				// Enable interrupt on receive.
				mrf_write_short(MRF_REG_SHORT_INTCON, 0xF7);
				// Turn on LED 2 to indicate capture is enabled.
				GPIOB_BSRR = GPIO_BS(13);
			} else {
				// Shut down the radio.
				mrf_write_short(MRF_REG_SHORT_RXMCR, 0x20);
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04);
				mrf_write_short(MRF_REG_SHORT_INTCON, 0xFF);
				mrf_analogue_off();
				// Turn off LED 2 to indicate capture is disabled.
				GPIOB_BSRR = GPIO_BR(13);
			}
			// Accept this request.
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static usb_ep0_memory_source_t mem_src;
	static uint8_t stash_buffer[2];

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_CHANNEL) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_SYMBOL_RATE) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_PAN_ID) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_MAC_ADDRESS) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_GET_PROMISCUOUS_FLAGS) {
		if (!pkt->value && !pkt->index) {
			*source = usb_ep0_memory_source_init(&mem_src, &promisc_flags, sizeof(promisc_flags));
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_INTERFACE) {
		if (!pkt->value && !pkt->index && pkt->length == 1) {
			stash_buffer[0] = 0;
			*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_INTERFACE) && pkt->request == USB_REQ_GET_STATUS) {
		if (!pkt->value && !pkt->index && pkt->length == 2) {
			stash_buffer[0] = 0;
			stash_buffer[1] = 0;
			*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 1);
			return USB_EP0_DISPOSITION_ACCEPT;
		} else {
			return USB_EP0_DISPOSITION_REJECT;
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

static void on_enter(void) {
	// Clear promiscuous mode flags.
	promisc_flags = 0;

	// Initialize the packet buffers.
	for (size_t i = 0; i < sizeof(perconfig.promisc_packets) / sizeof(*perconfig.promisc_packets) - 1; ++i) {
		perconfig.promisc_packets[i].next = &perconfig.promisc_packets[i + 1];
	}
	perconfig.promisc_packets[sizeof(perconfig.promisc_packets) / sizeof(*perconfig.promisc_packets) - 1].next = 0;
	first_free_packet = &perconfig.promisc_packets[0];
	first_captured_packet = last_captured_packet = 0;

	// Clear the dropped-packet flag.
	packet_dropped = false;

	// Initialize the radio.
	mrf_init();
	sleep_us(100);
	mrf_release_reset();
	mrf_common_init();
	while (GPIOC_IDR & (1 << 12));

	// Turn on LED 1.
	GPIOB_BSRR = GPIO_BS(12);

	// Enable external interrupt on MRF INT rising edge.
	exti_set_handler(12, &exti12_interrupt_vector);
	exti_map(12, 2); // Map PC12 to EXTI12
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR &= ~(1 << 12); // TR12 = 0; disable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Set up endpoint 1 with a 512-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(1, 512);
	usb_fifo_flush(1);
	usb_bi_in_init(1, 64, USB_BI_IN_EP_TYPE_BULK);
	usb_bi_in_set_std_halt(1, 0, 0, &handle_ep1_clear_halt);

	// Register endpoints callbacks.
	usb_ep0_cbs_push(&EP0_CBS);
}

static void on_exit(void) {
	// Unregister endpoints callbacks.
	usb_ep0_cbs_remove(&EP0_CBS);

	// Shut down IN endpoint 1.
	usb_bi_in_deinit(1);

	// Deallocate FIFOs.
	usb_fifo_disable(1);

	// Disable the external interrupt on MRF INT.
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	exti_set_handler(12, 0);

	// Turn off all LEDs.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Reset the radio.
	mrf_init();
}

const usb_configs_config_t PROMISCUOUS_CONFIGURATION = {
	.configuration = 3,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

