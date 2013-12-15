#include "buzzer.h"
#include "constants.h"
#include "enabled.h"
#include "estop.h"
#include "mrf.h"
#include <core_progmem.h>
#include <deferred.h>
#include <exception.h>
#include <format.h>
#include <gpio.h>
#include <init.h>
#include <rcc.h>
#include <registers/flash.h>
#include <registers/id.h>
#include <registers/mpu.h>
#include <registers/nvic.h>
#include <registers/otg_fs.h>
#include <registers/power.h>
#include <registers/scb.h>
#include <registers/systick.h>
#include <sleep.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unused.h>
#include <usb_configs.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_fifo.h>
#include <usb_ll.h>

static void stm32_main(void) __attribute__((noreturn));
static void nmi_vector(void);
static void service_call_vector(void);
static void system_tick_vector(void);
static void app_exception_early(void);
static void app_exception_late(bool core_written);
void adc_interrupt_vector(void);
void exti_dispatcher_0(void);
void exti_dispatcher_1(void);
void exti_dispatcher_2(void);
void exti_dispatcher_3(void);
void exti_dispatcher_4(void);
void exti_dispatcher_9_5(void);
void exti_dispatcher_15_10(void);
void timer5_interrupt_vector(void);
void timer6_interrupt_vector(void);
void timer7_interrupt_vector(void);

static char pstack[32768] __attribute__((section(".pstack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16] __attribute__((used, section(".exception_vectors"))) = {
	[0] = (fptr) (pstack + sizeof(pstack)),
	[1] = &stm32_main,
	[2] = &nmi_vector,
	[3] = &exception_hard_fault_vector,
	[4] = &exception_memory_manage_fault_vector,
	[5] = &exception_bus_fault_vector,
	[6] = &exception_usage_fault_vector,
	[11] = &service_call_vector,
	[14] = &deferred_fn_pendsv_handler,
	[15] = &system_tick_vector,
};

static const fptr interrupt_vectors[82] __attribute__((used, section(".interrupt_vectors"))) = {
	[6] = &exti_dispatcher_0,
	[7] = &exti_dispatcher_1,
	[8] = &exti_dispatcher_2,
	[9] = &exti_dispatcher_3,
	[10] = &exti_dispatcher_4,
	[18] = &adc_interrupt_vector,
	[23] = &exti_dispatcher_9_5,
	[40] = &exti_dispatcher_15_10,
	[50] = &timer5_interrupt_vector,
	[54] = &timer6_interrupt_vector,
	[55] = &timer7_interrupt_vector,
	[67] = &usb_ll_process,
};

static void nmi_vector(void) {
	abort();
}

static void service_call_vector(void) {
	for (;;);
}

static void system_tick_vector(void) {
	for (;;);
}

static const uint8_t DEVICE_DESCRIPTOR[18] = {
	18, // bLength
	USB_DTYPE_DEVICE, // bDescriptorType
	0, // bcdUSB LSB
	2, // bcdUSB MSB
	0, // bDeviceClass
	0, // bDeviceSubClass
	0, // bDeviceProtocol
	8, // bMaxPacketSize0
	(uint8_t) VENDOR_ID, // idVendor LSB
	VENDOR_ID >> 8, // idVendor MSB
	(uint8_t) PRODUCT_ID, // idProduct LSB
	PRODUCT_ID >> 8, // idProduct MSB
	1, // bcdDevice LSB
	1, // bcdDevice MSB
	STRING_INDEX_MANUFACTURER, // iManufacturer
	STRING_INDEX_PRODUCT, // iProduct
	STRING_INDEX_SERIAL, // iSerialNumber
	1, // bNumConfigurations
};

static const uint8_t STRING_ZERO[4] = {
	sizeof(STRING_ZERO),
	USB_DTYPE_STRING,
	0x09, 0x10, /* English (Canadian) */
};

static const usb_configs_config_t * const CONFIGURATIONS[] = {
	&ENABLED_CONFIGURATION,
	0
};

static const init_specs_t INIT_SPECS = {
	.hse_crystal = true,
	.hse_frequency = 8,
	.pll_frequency = 288,
	.sys_frequency = 144,
	.cpu_frequency = 144,
	.apb1_frequency = 36,
	.apb2_frequency = 72,
	.exception_core_writer = &core_progmem_writer,
	.exception_app_cbs = {
		.early = &app_exception_early,
		.late = &app_exception_late,
	},
};

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_SET_ADDRESS) {
		// This request must have a valid address as its value, an index of zero, and occur while the device is unconfigured.
		if (pkt->value > 127 || pkt->index || usb_configs_get_current()) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Lock in the address; the hardware knows to stay on address zero until the status stage is complete and, in fact, *FAILS* if the address is locked in later!
		OTG_FS_DCFG.DAD = pkt->value;

		// Initialize or deinitialize the configuration handler module depending on the assigned address.
		if (pkt->value) {
			usb_configs_init(CONFIGURATIONS);
		} else {
			usb_configs_deinit();
		}

		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[25];
	static union {
		usb_ep0_memory_source_t mem_src;
		usb_ep0_string_descriptor_source_t string_src;
	} src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_GET_STATUS) {
		// This request must have value and index set to zero.
		if (pkt->value || pkt->index) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// We do not support remote wakeup, so bit 1 is always set to zero.
		// We are always *effectively* bus-powered (we can be target-powered, but might as well be so only when the bus is disconnected), so bit 0 is always set to zero.
		stash_buffer[0] = 0;
		stash_buffer[1] = 0;
		*source = usb_ep0_memory_source_init(&src.mem_src, stash_buffer, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_GET_DESCRIPTOR) {
		uint8_t type = pkt->value >> 8;
		uint8_t index = pkt->value;
		switch (type) {
			case USB_DTYPE_DEVICE:
			{
				// GET DESCRIPTOR(DEVICE)
				if (index || pkt->index) {
					return USB_EP0_DISPOSITION_REJECT;
				}
				*source = usb_ep0_memory_source_init(&src.mem_src, DEVICE_DESCRIPTOR, sizeof(DEVICE_DESCRIPTOR));
				return USB_EP0_DISPOSITION_ACCEPT;
			}

			case USB_DTYPE_CONFIGURATION:
			{
				// GET DESCRIPTOR(CONFIGURATION)
				if (pkt->index) {
					return USB_EP0_DISPOSITION_REJECT;
				}

				const uint8_t *descriptor = 0;
				switch (index) {
					case 0: descriptor = ENABLED_CONFIGURATION_DESCRIPTOR; break;
				}
				if (descriptor) {
					size_t total_length = descriptor[2] | (descriptor[3] << 8);
					*source = usb_ep0_memory_source_init(&src.mem_src, descriptor, total_length);
					return USB_EP0_DISPOSITION_ACCEPT;
				} else {
					return USB_EP0_DISPOSITION_REJECT;
				}
			}

			case USB_DTYPE_STRING:
			{
				// GET DESCRIPTOR(STRING)
				if (!index && !pkt->index) {
					*source = usb_ep0_memory_source_init(&src.mem_src, STRING_ZERO, sizeof(STRING_ZERO));
					return USB_EP0_DISPOSITION_ACCEPT;
				} else if (pkt->index == 0x1009 /* English (Canadian) */) {
					const char *string = 0;
					switch (index) {
						case STRING_INDEX_MANUFACTURER: string = u8"UBC Thunderbots Small Size Team"; break;
						case STRING_INDEX_PRODUCT: string = u8"Radio Base Station"; break;
						case STRING_INDEX_RADIO_OFF: string = u8"Radio Off"; break;
						case STRING_INDEX_NORMAL: string = u8"Normal Mode"; break;
						case STRING_INDEX_PROMISCUOUS: string = u8"Promiscuous Mode"; break;
						case STRING_INDEX_SERIAL:
							formathex32((char *) stash_buffer + 0, U_ID.H);
							formathex32((char *) stash_buffer + 8, U_ID.M);
							formathex32((char *) stash_buffer + 16, U_ID.L);
							((char *) stash_buffer)[24] = '\0';
							string = (const char *) stash_buffer;
							break;
					}
					if (string) {
						*source = usb_ep0_string_descriptor_source_init(&src.string_src, string);
						return USB_EP0_DISPOSITION_ACCEPT;
					} else {
						return USB_EP0_DISPOSITION_REJECT;
					}
				} else {
					return USB_EP0_DISPOSITION_REJECT;
				}
			}

			case USB_DTYPE_INTERFACE:
			case USB_DTYPE_ENDPOINT:
			case USB_DTYPE_DEVICE_QUALIFIER:
			case USB_DTYPE_OTHER_SPEED_CONFIGURATION:
			case USB_DTYPE_INTERFACE_POWER:
			{
				// GET DESCRIPTOR(…)
				// These are either not present or are meant to be requested through GET DESCRIPTOR(CONFIGURATION).
				return USB_EP0_DISPOSITION_REJECT;
			}

			default:
			{
				// GET DESCRIPTOR(unknown)
				// Other descriptors may be handled by other layers.
				return USB_EP0_DISPOSITION_NONE;
			}
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t GLOBAL_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

static void handle_usb_reset(void) {
	// Turn off the buzzer.
	buzzer_stop();

	// Shut down the control transfer layer, if the device was already active (implying the control transfer layer had been initialized).
	if (usb_ll_get_state() == USB_LL_STATE_ACTIVE) {
		usb_configs_deinit();
		usb_ep0_deinit();
	}
	
	// Configure receive FIFO and endpoint 0 transmit FIFO sizes.
	usb_fifo_init(512, 64);
}

static void handle_usb_enumeration_done(void) {
	usb_ep0_init(8);
	usb_ep0_cbs_push(&GLOBAL_CBS);
}

static void app_exception_early(void) {
	// Power down the USB engine to disconnect from the host.
	OTG_FS_GCCFG.PWRDWN = 0;

	// Turn the three LEDs on.
	gpio_set_reset_mask(GPIOB, 7 << 12, 0);
}

static void app_exception_late(bool core_written) {
	// Show flashing lights.
	for (;;) {
		gpio_set_reset_mask(GPIOB, 0, 7 << 12);
		sleep_ms(500);
		gpio_set_reset_mask(GPIOB, core_written ? (7 << 12) : (1 << 12), 0);
		sleep_ms(500);
	}
}

static void stm32_main(void) {
	init_chip(&INIT_SPECS);

	// Initialize subsystems.
	buzzer_init();

	// Set up pins.
	rcc_enable(AHB1, GPIOA);
	rcc_enable(AHB1, GPIOB);
	rcc_enable(AHB1, GPIOC);
	rcc_enable(AHB1, GPIOD);
	rcc_reset(AHB1, GPIOA);
	rcc_reset(AHB1, GPIOB);
	rcc_reset(AHB1, GPIOC);
	rcc_reset(AHB1, GPIOD);
	// PA15 = MRF /CS, start deasserted
	// PA14/PA13 = alternate function SWD
	// PA12/PA11 = alternate function OTG FS
	// PA10/PA9/PA8/PA7/PA6 = N/C
	// PA5/PA4 = shorted to VDD
	// PA3 = shorted to VSS
	// PA2 = alternate function TIM2 buzzer
	// PA1/PA0 = shorted to VDD
	GPIOA.ODR = 0b1000000000110011;
	GPIOA.OSPEEDR = 0b01000001010000000000000000000000;
	GPIOA.PUPDR = 0b00100100000000000000000000000000;
	GPIOA.AFRH = 0b00000000000010101010000000000000;
	GPIOA.AFRL = 0b00000000000000000000000100000000;
	GPIOA.MODER = 0b01101010100101010101010101100101;
	// PB15 = N/C
	// PB14 = LED 3
	// PB13 = LED 2
	// PB12 = LED 1
	// PB11/PB10 = N/C
	// PB9/PB8 = shorted to VSS
	// PB7 = MRF /reset, start asserted
	// PB6 = MRF wake, start deasserted
	// PB5 = alternate function MRF MOSI
	// PB4 = alternate function MRF MISO
	// PB3 = alternate function MRF SCK
	// PB2 = BOOT1, hardwired low
	// PB1 = run switch input, analogue
	// PB0 = run switch positive supply, start low
	GPIOB.ODR = 0b0111000000000000;
	GPIOB.OSPEEDR = 0b00000000000000000000010001000000;
	GPIOB.PUPDR = 0b00000000000000000000001000000000;
	GPIOB.AFRH = 0b00000000000000000000000000000000;
	GPIOB.AFRL = 0b00000000010101010101000000000000;
	GPIOB.MODER = 0b01010101010101010101101010011101;
	// PC15/PC14/PC13 = N/C
	// PC12 = MRF INT, input
	// PC11/PC10/PC9/PC8/PC7/PC6 = N/C
	// PC5 = run switch negative supply, always low
	// PC4/PC3/PC2/PC1/PC0 = N/C
	GPIOC.ODR = 0b0000000000000000;
	GPIOC.OSPEEDR = 0b00000000000000000000000000000000;
	GPIOC.PUPDR = 0b00000000000000000000000000000000;
	GPIOC.AFRH = 0b00000000000000000000000000000000;
	GPIOC.AFRL = 0b00000000000000000000000000000000;
	GPIOC.MODER = 0b01010100010101010101010101010101;
	// PD15/PD14/PD13/PD12/PD11/PD10/PD9/PD8/PD7/PD6/PD5/PD4/PD3 = unimplemented on package
	// PD2 = N/C
	// PD1/PD0 = unimplemented on package
	GPIOD.ODR = 0b0000000000000000;
	GPIOD.OSPEEDR = 0b00000000000000000000000000000000;
	GPIOD.PUPDR = 0b00000000000000000000000000000000;
	GPIOD.AFRH = 0b00000000000000000000000000000000;
	GPIOD.AFRL = 0b00000000000000000000000000000000;
	GPIOD.MODER = 0b01010101010101010101010101010101;
	// PE/PF/PG = unimplemented on this package
	// PH15/PH14/PH13/PH12/PH11/PH10/PH9/PH8/PH7/PH6/PH5/PH4/PH3/PH2 = unimplemented on this package
	// PH1 = OSC_OUT (not configured via GPIO registers)
	// PH0 = OSC_IN (not configured via GPIO registers)
	// PI15/PI14/PI13/PI12 = unimplemented
	// PI11/PI10/PI9/PI8/PI7/PI6/PI5/PI4/PI3/PI2/PI1/PI0 = unimplemented on this package

	// Initialize more subsystems.
	estop_init();

	// Wait a bit.
	sleep_ms(100);

	// Turn off LEDs.
	gpio_set_reset_mask(GPIOB, 0, 7 << 12);

	// Initialize USB.
	usb_ll_attach(&handle_usb_reset, &handle_usb_enumeration_done, 0);
	NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt

	// Switch to unprivileged mode.
	asm volatile(
			"msr control, %[control_value]\n\t"
			"dsb\n\t"
			"isb\n\t"
			:
			: [control_value] "r" (0b011 /* FPCA = 0, SPSEL = 1, nPRIV = 1 */));

	// Now wait forever handling activity in interrupt handlers.
	for (;;) {
		asm volatile("wfi");
	}
}

