#include "configs.h"
#include "constants.h"
#include "interrupt.h"
#include "mrf.h"
#include "rcc.h"
#include "registers.h"
#include "stdint.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

const uint8_t CONFIGURATION_DESCRIPTOR6[] = {
	9, // bLength
	2, // bDescriptorType
	25, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	6, // bConfigurationValue
	8, // iConfiguration
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
	0x03, // bmAttributes
	1, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval
};

static void exti12_interrupt_vector(void) {
	// Clear the interrupt
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	// Check the INT pin level
	bool int_level = !!(GPIOC_IDR & (1 << 12));

	// Display the INT pin level on LED 2
	GPIOB_BSRR = int_level ? 2 << 12 : 2 << (12 + 16);

	// Check if the transmit FIFO has any empty space
	if (OTG_FS_DTXFSTS1 & 0xFFFF) {
		// Space is available; queue a packet
		OTG_FS_DIEPTSIZ1 += (1 << 19) // PKTCNT += 1; increment count of packets to send
			| (1 << 0); // XFRSIZ += 1; increment count of bytes to send
		OTG_FS_DIEPCTL1 |= (1 << 31) // EPENA = 1; start transmission on this endpoint
			| (1 << 26); // CNAK = 1; clear NAk flag
		OTG_FS_FIFO[1][0] = int_level ? 1 : 0;
	} else {
		// Space is not available; light LED 3 and wait for empty space
		GPIOB_BSRR = 4 << 12;
		OTG_FS_DIEPEMPMSK |= 1 << 1; // INEPTXFEM1 = 1; take interrupt on IN endpoint 1 TX FIFO empty
	}
}

static void on_ep1_in_interrupt(void) {
	if ((OTG_FS_DIEPEMPMSK & (1 << 1) /* INEPTXFEM1 */) && (OTG_FS_DIEPINT1 & (1 << 7) /* TXFE */)) {
		// We were waiting to be notified about the FIFO becoming empty, and now it has
		// Push a packet
		OTG_FS_DIEPTSIZ1 += (1 << 19) // PKTCNT += 1; increment count of packets to send
			| (1 << 0); // XFRSIZ += 1; increment count of bytes to send
		OTG_FS_FIFO[1][0] = !!(GPIOC_IDR & (1 << 12));
		// Stop asking to be interrupted about FIFO empty
		OTG_FS_DIEPEMPMSK &= ~(1 << 1); // INEPTXFEM1 = 0; do not interrupt on IN endpoint 1 TX FIFO empty
		GPIOB_BSRR = 4 << (12 + 16);
	} else if (OTG_FS_DIEPINT1 & (1 << 0) /* XFRC */) {
		// We don’t actually care about transfer complete notification
		OTG_FS_DIEPINT1 = 1 << 0; // XFRC = 1; clear transfer complete interrupt flag
	}
}

static void on_enter(void) {
	// Initialize radio
	mrf_init();

	// Turn on LED 1
	GPIOB_BSRR = 1 << 12;

	// Configure MRF INT (PC12) as an external interrupt
	interrupt_exti12_handler = &exti12_interrupt_vector;
	rcc_enable(APB2, 14);
	SYSCFG_EXTICR[12 / 4] = (SYSCFG_EXTICR[12 / 4] & ~(15 << (12 % 4))) | (2 << (12 % 4)); // EXTI12 = 2; map PC12 to EXTI12
	rcc_disable(APB2, 14);
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR |= 1 << 12; // TR12 = 1; enable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Set up endpoint 1 (interrupt IN)
	usb_in_set_callback(1, &on_ep1_in_interrupt);
	OTG_FS_DIEPTXF1 =
		(16 << 16) // INEPTXFD = 16; allocate 16 words of FIFO space for this FIFO
		| (usb_application_fifo_offset() << 0); // INEPTXSA = offset; place this FIFO after the endpoint-zero FIFOs
	OTG_FS_DIEPCTL1 =
		(0 << 31) // EPENA = 0; do not start transmission on this endpoint
		| (0 << 30) // EPDIS = 0; do not disable this endpoint at this time
		| (1 << 28) // SD0PID = 1; set data PID to 0
		| (1 << 27) // SNAK = 1; set NAK flag
		| (0 << 26) // CNAK = 0; do not clear NAK flag
		| (1 << 22) // TXFNUM = 1; use transmit FIFO number 1
		| (0 << 21) // STALL = 0; do not stall traffic
		| (3 << 18) // EPTYP = 3; interrupt endpoint
		| (1 << 15) // USBAEP = 1; endpoint is active in this configuration
		| (1 << 0); // MPSIZ = 1; maximum packet size is 1 byte
	while (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */));
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));
	OTG_FS_GRSTCTL = (OTG_FS_GRSTCTL & 0x7FFFF808) // Reserved
		| (1 << 6) // TXFNUM = 1; flush transmit FIFO #1
		| (1 << 5); // TXFFLSH = 1; flush transmit FIFO
	while (OTG_FS_GRSTCTL & (1 << 5) /* TXFFLSH */);
	OTG_FS_DIEPINT1 = OTG_FS_DIEPINT1; // Clear all pending interrupts for IN endpoint 1
	OTG_FS_DAINTMSK |= 1 << 1; // IEPM1 = 1; enable interrupts for IN endpoint 1

	// Display the current level of INT on LED 3
	bool int_level = !!(GPIOC_IDR & (1 << 12));
	GPIOB_BSRR = int_level ? 2 << 12 : 2 << (12 + 16);
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
	if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_CONTROL_LINES && !(value & 0b1111111111111100) && !index) {
		GPIOB_BSRR = (value & (1 << 0)) ? 1 << 7 : 1 << (7 + 16);
		GPIOB_BSRR = (value & (1 << 1)) ? 1 << 6 : 1 << (6 + 16);
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_SHORT_REGISTER && value <= 0xFF && index <= 0x3F) {
		mrf_write_short(index, value);
		*accept = true;
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_LONG_REGISTER && value <= 0xFF && index <= 0x038F && !(0x02C0 <= index && index <= 0x02FF)) {
		mrf_write_long(index, value);
		*accept = true;
		return true;
	} else {
		return false;
	}
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length __attribute__((unused)), usb_ep0_source_t **source) {
	static uint8_t buffer[1];
	static usb_ep0_memory_source_t mem_src;

	if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_CONTROL_LINES && !value && !index) {
		buffer[0] = 0;
		if (GPIOB_ODR & (1 << 7)) {
			buffer[0] |= 1 << 0;
		}
		if (GPIOB_ODR & (1 << 6)) {
			buffer[0] |= 1 << 1;
		}
		if (GPIOC_IDR & (1 << 12)) {
			buffer[0] |= 1 << 2;
		}
		*source = usb_ep0_memory_source_init(&mem_src, buffer, 1);
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_SHORT_REGISTER && !value && index <= 0x3F) {
		buffer[0] = mrf_read_short(index);
		*source = usb_ep0_memory_source_init(&mem_src, buffer, 1);
		return true;
	} else if (request_type == (USB_STD_REQ_TYPE_IN | USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_GET_LONG_REGISTER && !value && index <= 0x038F && !(0x02C0 <= index && index <= 0x02FF)) {
		buffer[0] = mrf_read_long(index);
		*source = usb_ep0_memory_source_init(&mem_src, buffer, 1);
		return true;
	} else {
		return false;
	}
}

const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS6 = {
	.configuration = 6,
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

