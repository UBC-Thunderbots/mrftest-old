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
#include "usb_fifo.h"

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
	bool int_level = !!(IDR_X(GPIOC_IDR) & (1 << 12));

	// Display the INT pin level on LED 2
	GPIOB_BSRR = int_level ? GPIO_BS(12) : GPIO_BR(12);

	// Check if the transmit FIFO has any empty space
	if (INEPTFSAV_X(OTG_FS_DTXFSTS1)) {
		// Space is available; queue a packet
		OTG_FS_DIEPTSIZ1 += PKTCNT(1) // Increment count of packets to send
			| XFRSIZ(1); // Increment count of bytes to send
		OTG_FS_DIEPCTL1 |= EPENA // Start transmission on this endpoint
			| CNAK; // Clear NAk flag
		OTG_FS_FIFO[1][0] = int_level ? 1 : 0;
	} else {
		// Space is not available; light LED 3 and wait for empty space
		GPIOB_BSRR = GPIO_BS(12) | GPIO_BS(13) | GPIO_BS(14);
		OTG_FS_DIEPEMPMSK |= INEPTXFEM(1 << 1); // Take interrupt on IN endpoint 1 TX FIFO empty
	}
}

static void on_ep1_in_interrupt(void) {
	if ((OTG_FS_DIEPEMPMSK & INEPTXFEM(1 << 1)) && (OTG_FS_DIEPINT1 & TXFE)) {
		// We were waiting to be notified about the FIFO becoming empty, and now it has
		// Push a packet
		OTG_FS_DIEPTSIZ1 += PKTCNT(1) // Increment count of packets to send
			| XFRSIZ(1); // Increment count of bytes to send
		OTG_FS_FIFO[1][0] = !!(IDR_X(GPIOC_IDR) & (1 << 12));
		// Stop asking to be interrupted about FIFO empty
		OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << 1); // Do not interrupt on IN endpoint 1 TX FIFO empty
		GPIOB_BSRR = GPIO_BR(14);
	} else if (OTG_FS_DIEPINT1 & XFRC) {
		// We don’t actually care about transfer complete notification
		OTG_FS_DIEPINT1 = XFRC; // Clear transfer complete interrupt flag
	}
}

static void on_enter(void) {
	// Initialize radio
	mrf_init();

	// Turn on LED 1
	GPIOB_BSRR = GPIO_BS(12);

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
	OTG_FS_DIEPCTL1 =
		0 // EPENA = 0; do not start transmission on this endpoint
		| 0 // EPDIS = 0; do not disable this endpoint at this time
		| SD0PID // Set data PID to 0
		| SNAK // Set NAK flag
		| 0 // CNAK = 0; do not clear NAK flag
		| DIEPCTL_TXFNUM(1) // Use transmit FIFO number 1
		| 0 // STALL = 0; do not stall traffic
		| EPTYP(3) // Interrupt endpoint
		| USBAEP // Endpoint is active in this configuration
		| MPSIZ(1); // Maximum packet size is 1 byte
	while (!(OTG_FS_DIEPCTL1 & NAKSTS));
	usb_fifo_set_size(1, 16); // Allocate 16 words of FIFO space for this FIFO
	usb_fifo_flush(1);
	OTG_FS_DIEPINT1 = OTG_FS_DIEPINT1; // Clear all pending interrupts for IN endpoint 1
	OTG_FS_DAINTMSK |= IEPM(1 << 1); // Enable interrupts for IN endpoint 1

	// Display the current level of INT on LED 3
	bool int_level = !!(IDR_X(GPIOC_IDR) & (1 << 12));
	GPIOB_BSRR = int_level ? GPIO_BS(13) : GPIO_BR(13);
}

static void on_exit(void) {
	// Shut down endpoint 1
	if (OTG_FS_DIEPCTL1 & EPENA) {
		if (!(OTG_FS_DIEPCTL1 & NAKSTS)) {
			OTG_FS_DIEPCTL1 |= SNAK; // Start NAKing traffic
			while (!(OTG_FS_DIEPCTL1 & NAKSTS));
		}
		OTG_FS_DIEPCTL1 |= EPDIS; // Disable endpoint
		while (OTG_FS_DIEPCTL1 & EPENA);
	}
	OTG_FS_DIEPCTL1 = 0;
	OTG_FS_DAINTMSK &= ~IEPM(1); // Disable general interrupts for IN endpoint 1
	OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << 1); // Disable FIFO empty interrupts for IN endpoint 1
	usb_in_set_callback(1, 0);

	// Deallocate FIFOs.
	usb_fifo_reset();

	// Disable the external interrupt on MRF INT
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	interrupt_exti12_handler = 0;

	// Turn off all LEDs
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Reset the radio
	mrf_init();
}

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept) {
	if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_SET_CONTROL_LINES && !(value & 0b1111111111111100) && !index) {
		GPIOB_BSRR = (value & (1 << 0)) ? GPIO_BS(7) : GPIO_BR(7);
		GPIOB_BSRR = (value & (1 << 1)) ? GPIO_BS(6) : GPIO_BR(6);
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
		if (ODR_X(GPIOB_ODR) & (1 << 7)) {
			buffer[0] |= 1 << 0;
		}
		if (ODR_X(GPIOB_ODR) & (1 << 6)) {
			buffer[0] |= 1 << 1;
		}
		if (IDR_X(GPIOC_IDR) & (1 << 12)) {
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

