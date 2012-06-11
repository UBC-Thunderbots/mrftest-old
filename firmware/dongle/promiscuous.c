#include "config.h"
#include "configs.h"
#include "interrupt.h"
#include "mrf.h"
#include "perconfig.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"
#include "stddef.h"
#include "stdint.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

const uint8_t CONFIGURATION_DESCRIPTOR3[] = {
	9, // bLength
	2, // bDescriptorType
	25, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	3, // bConfigurationValue
	5, // iConfiguration
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
static bool packet_dropped, usb_active, zlp_pending;

static void push_data(void) {
	if (usb_active) {
		// A packet is already going over USB; wait until that’s finished first
		return;
	}

	if (zlp_pending) {
		// A prior transfer needed a zero-length packet to be added on; do that now
		OTG_FS_DIEPTSIZ1 =
			(1 << 19) // PKTCNT = 1; send one packet
			| (0 << 0); // XFRSIZ = 0; send zero bytes
		OTG_FS_DIEPCTL1 |=
			(1 << 31) // EPENA = 1; enable endpoint
			| (1 << 26); // CNAK = 1; clear NAK flag
		zlp_pending = false;
		usb_active = true;
		return;
	}

	if (!first_captured_packet) {
		// No captured packets to send; wait until we have one first
		return;
	}

	// Pop a packet from the queue
	promisc_packet_t *packet = first_captured_packet;
	first_captured_packet = packet->next;
	if (!first_captured_packet) {
		last_captured_packet = 0;
	}
	packet->next = 0;

	// Push this captured packet into the USB FIFO
	OTG_FS_DIEPTSIZ1 =
		(((packet->length + 63) / 64) << 19) // PKTCNT = n; send the proper number of packets (this does not include any ZLP, which must be in a separate “transfer” as far as the STM32F4’s USB engine is concerned)
		| (packet->length << 0); // XFRSIZ = n; send the proper number of bytes
	OTG_FS_DIEPCTL1 |=
		(1 << 31) // EPENA = 1; enable endpoint
		| (1 << 26); // CNAK = 1; clear NAK flag
	for (size_t i = 0; i < (packet->length + 3U) / 4U; ++i) {
		uint32_t word = packet->data[i * 4] | (packet->data[i * 4 + 1] << 8) | (packet->data[i * 4 + 2] << 16) | (packet->data[i * 4 + 3] << 24);
		OTG_FS_FIFO[1][0] = word;
	}
	usb_active = true;

	// Decide if a zero-length packet will be needed after this
	zlp_pending = !(packet->length % 64);

	// Push this consumed packet buffer into the free list
	packet->next = first_free_packet;
	first_free_packet = packet;
}

static void exti12_interrupt_vector(void) {
	// Clear the interrupt
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	while (GPIOC_IDR & (1 << 12) /* PC12 */) {
		// Check outstanding interrupts
		uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
		if (intstat & (1 << 3)) {
			// RXIF = 1; packet received
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
			uint8_t rxfifo_frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO); // Need to read this here even if no packet buffer because this read also re-enables the receiver
			promisc_packet_t *packet = first_free_packet;
			if (packet) {
#warning proper packet filtering when radio filters do not match capture flags exactly
				first_free_packet = packet->next;
				packet->next = 0;
				packet->length = 1 /* Flags */ + 1 /* Channel */ + rxfifo_frame_length /* MAC header + data + FCS */ + 1 /* LQI */ + 1 /* RSSI */;
				packet->data[0] = packet_dropped ? 0x01 : 0x00;
				packet_dropped = false;
				packet->data[1] = config.channel;
				for (size_t i = 0; i < rxfifo_frame_length + 2U; ++i) {
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
			// Toggle LED 3 to show reception
			GPIOB_ODR ^= 4 << 12;
		}
	}
}

static void on_ep1_in_interrupt(void) {
	if (OTG_FS_DIEPINT1 & (1 << 0) /* XFRC */) {
		// On transfer complete, go see if we have another captured packet to send
		OTG_FS_DIEPINT1 = 1 << 0; // XFRC = 1; clear transfer complete interrupt flag
		usb_active = false;
		push_data();
	}
}

static void on_enter(void) {
	// Clear promiscuous mode flags
	promisc_flags = 0;

	// Initialize the packet buffers
	for (size_t i = 0; i < sizeof(perconfig.promisc_packets) / sizeof(*perconfig.promisc_packets) - 1; ++i) {
		perconfig.promisc_packets[i].next = &perconfig.promisc_packets[i + 1];
	}
	perconfig.promisc_packets[sizeof(perconfig.promisc_packets) / sizeof(*perconfig.promisc_packets) - 1].next = 0;
	first_free_packet = &perconfig.promisc_packets[0];
	first_captured_packet = last_captured_packet = 0;

	// Clear the dropped-packet flag
	packet_dropped = false;

	// Note that no packet is currently being sent over USB, nor that any zero-length packet is pending
	usb_active = false;
	zlp_pending = false;

	// Initialize the radio
	mrf_init();
	sleep_100us(1);
	mrf_release_reset();
	mrf_common_init();
	while (GPIOC_IDR & (1 << 12));

	// Turn on LED 1
	GPIOB_BSRR = 1 << 12;

	// Enable external interrupt on MRF INT rising edge
	interrupt_exti12_handler = &exti12_interrupt_vector;
	rcc_enable(APB2, 14);
	SYSCFG_EXTICR[12 / 4] = (SYSCFG_EXTICR[12 / 4] & ~(15 << (12 % 4))) | (2 << (12 % 4)); // EXTI12 = 2; map PC12 to EXTI12
	rcc_disable(APB2, 14);
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR &= ~(1 << 12); // TR12 = 0; disable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Set up endpoint 1 (interrupt IN)
	usb_in_set_callback(1, &on_ep1_in_interrupt);
	OTG_FS_DIEPTXF1 =
		(128 << 16) // INEPTXFD = 128; allocate 128 words of FIFO space for this FIFO; this is larger than any transfer we will ever send, so we *never* need to deal with a full FIFO!
		| (usb_application_fifo_offset() << 0); // INEPTXSA = offset; place this FIFO after the endpoint-zero FIFOs
	OTG_FS_DIEPCTL1 =
		(0 << 31) // EPENA = 0; do not start transmission on this endpoint
		| (0 << 30) // EPDIS = 0; do not disable this endpoint at this time
		| (1 << 28) // SD0PID = 1; set data PID to 0
		| (1 << 27) // SNAK = 1; set NAK flag
		| (0 << 26) // CNAK = 0; do not clear NAK flag
		| (1 << 22) // TXFNUM = 1; use transmit FIFO number 1
		| (0 << 21) // STALL = 0; do not stall traffic
		| (2 << 18) // EPTYP = 2; bulk endpoint
		| (1 << 15) // USBAEP = 1; endpoint is active in this configuration
		| (64 << 0); // MPSIZ = 64; maximum packet size is 64 bytes
	while (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */));
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));
	OTG_FS_GRSTCTL = (OTG_FS_GRSTCTL & 0x7FFFF808) // Reserved
		| (1 << 6) // TXFNUM = 1; flush transmit FIFO #1
		| (1 << 5); // TXFFLSH = 1; flush transmit FIFO
	while (OTG_FS_GRSTCTL & (1 << 5) /* TXFFLSH */);
	OTG_FS_DIEPINT1 = OTG_FS_DIEPINT1; // Clear all pending interrupts for IN endpoint 1
	OTG_FS_DAINTMSK |= 1 << 1; // IEPM1 = 1; enable interrupts for IN endpoint 1
}

static void on_exit(void) {
	// Shut down endpoint 1
	if (OTG_FS_DIEPCTL1 & (1 << 31) /* EPENA */) {
		if (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */)) {
			OTG_FS_DIEPCTL1 |= 1 << 27; // SNAK = 1; start NAKing traffic
			while (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */));
		}
		OTG_FS_DIEPCTL1 |= 1 << 30; // EPDIS = 1; disable endpoint
		while (OTG_FS_DIEPCTL1 & (1 << 31) /* EPENA */);
	}
	OTG_FS_DIEPCTL1 = 0;
	OTG_FS_DAINTMSK &= ~(1 << 1); // IEPM1 = 0; disable general interrupts for IN endpoint 1
	OTG_FS_DIEPEMPMSK &= ~(1 << 1); // INEPTXFEM1 = 0; disable FIFO empty interrupts for IN endpoint 1
	usb_in_set_callback(1, 0);

	// Disable the external interrupt on MRF INT
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	interrupt_exti12_handler = 0;

	// Turn off all LEDs
	GPIOB_BSRR = 7 << (12 + 16);

	// Reset the radio
	mrf_init();
}

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept) {
	if (request_type == 0x40 && request == 0x0B && !index) {
		// SET PROMISCUOUS FLAGS
		*accept = false;
		// Sanity check: flags 8 through 15 are reserved and must be zero
		if (value & 0xFF00) {
			return true;
		}
		// Check if the application actually wants *ANY* packets
		if (value & 0xF0) {
			// Sanity check: if flag 0 (acknowledge) is set and one of bits 4 through 7 (accept some type of frame) is set…
			if ((value & 0x0001) && (value & 0xF0)) {
				// … either exactly one or all three of bits 4 through 6 must be set to 1…
				if ((value & 0x70) != 0x10 && (value & 0x70) != 0x20 && (value & 0x70) != 0x40 && (value & 0x70) != 0x70) {
					return true;
				}
				// … either none or all of bits 1, 2, and 7 must be set to 1…
				if ((value & 0x86) != 0x00 && (value & 0x86) != 0x86) {
					return true;
				}
				// … and if bit 7 is set to 1, bits 4 through 6 must also all be set to 1
				if ((value & 0x80) && (value & 0x70) != 0x70) {
					return true;
				}
			}
			// This set of flags is acceptable; save
			promisc_flags = value;
			// Disable all packet reception
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04);
			// Install the new flags
			mrf_write_short(MRF_REG_SHORT_RXMCR,
					((value & (1 << 0)) ? 0 : (1 << 5))
					| ((value & (1 << 3)) ? (1 << 1) : 0)
					| ((value & 0x86) ? (1 << 0) : 0));
			if ((value & 0xF0) == 0x10) {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x64);
			} else if ((value & 0xF0) == 0x20) {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x68);
			} else if ((value & 0xF0) == 0x40) {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x62);
			} else {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x60);
			}
			// Set analogue path appropriately based on whether ACKs are being generated and whether any packets are desired
			if (value & 0x01) {
				mrf_analogue_txrx();
			} else {
				mrf_analogue_rx();
			}
			// Re-enable packet reception
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00);
			// Enable interrupt on receive
			mrf_write_short(MRF_REG_SHORT_INTCON, 0xF7);
			// Turn on LED 2 to indicate capture is enabled
			GPIOB_BSRR = 2 << 12;
		} else {
			// Shut down the radio
			mrf_write_short(MRF_REG_SHORT_RXMCR, 0x20);
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04);
			mrf_write_short(MRF_REG_SHORT_INTCON, 0xFF);
			mrf_analogue_off();
			// Turn off LED 2 to indicate capture is disabled
			GPIOB_BSRR = 2 << (12 + 16);
		}
		// Accept this request
		*accept = true;
		return true;
	} else {
		return false;
	}
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length __attribute__((unused)), usb_ep0_source_t **source) {
	static usb_ep0_memory_source_t mem_src;

	if (request_type == 0xC0 && request == 0x00 && !value && !index) {
		// GET CHANNEL
		*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
		return true;
	} else if (request_type == 0xC0 && request == 0x02 && !value && !index) {
		// GET SYMBOL RATE
		*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
		return true;
	} else if (request_type == 0xC0 && request == 0x04 && !value && !index) {
		// GET PAN ID
		*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
		return true;
	} else if (request_type == 0xC0 && request == 0x06 && !value && !index) {
		// GET MAC ADDRESS
		*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
		return true;
	} else if (request_type == 0xC0 && request == 0x0A && !value && !index) {
		// GET PROMISCUOUS FLAGS
		*source = usb_ep0_memory_source_init(&mem_src, &promisc_flags, sizeof(promisc_flags));
		return true;
	} else {
		return false;
	}
}

const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS3 = {
	.configuration = 3,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 1,
	.can_enter = 0,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = 0,
};

