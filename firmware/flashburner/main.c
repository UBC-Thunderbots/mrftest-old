#include "autonomous.h"
#include "constants.h"
#include "exti.h"
#include "gpio.h"
#include "host_controlled.h"
#include "idle.h"
#include "spi.h"
#include "uart.h"
#include <core_progmem.h>
#include <deferred.h>
#include <exception.h>
#include <format.h>
#include <gpio.h>
#include <rcc.h>
#include <registers/exti.h>
#include <registers/flash.h>
#include <registers/id.h>
#include <registers/mpu.h>
#include <registers/nvic.h>
#include <registers/otg_fs.h>
#include <registers/scb.h>
#include <registers/syscfg.h>
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

static void stm32_main(void) __attribute__((naked, noreturn));
static void nmi_vector(void);
static void service_call_vector(void);
static void system_tick_vector(void);
void exti_dispatcher_0(void);
void exti_dispatcher_1(void);
void exti_dispatcher_2(void);
void exti_dispatcher_3(void);
void exti_dispatcher_4(void);
void exti_dispatcher_9_5(void);
void exti_dispatcher_15_10(void);
void timer5_interrupt_vector(void);
void usart1_interrupt_vector(void);

static char mstack[32768] __attribute__((section(".mstack")));
static char pstack[32768] __attribute__((section(".pstack"), used));

typedef void (*fptr)(void);
static const fptr exception_vectors[16] __attribute__((used, section(".exception_vectors"))) = {
	[0] = (fptr) (mstack + sizeof(mstack)),
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
	[23] = &exti_dispatcher_9_5,
	[37] = &usart1_interrupt_vector,
	[40] = &exti_dispatcher_15_10,
	[50] = &timer5_interrupt_vector,
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

volatile uint64_t bootload_flag;

static const uint8_t DEVICE_DESCRIPTOR[18] = {
	18, // bLength
	USB_DTYPE_DEVICE, // bDescriptorType
	0, // bcdUSB LSB
	2, // bcdUSB MSB
	0, // bDeviceClass
	0, // bDeviceSubClass
	0, // bDeviceProtocol
	8, // bMaxPacketSize0
	(uint8_t) FLASH_BURNER_VID, // idVendor LSB
	FLASH_BURNER_VID >> 8, // idVendor MSB
	(uint8_t) FLASH_BURNER_PID, // idProduct LSB
	FLASH_BURNER_PID >> 8, // idProduct MSB
	0, // bcdDevice LSB
	1, // bcdDevice MSB
	STRING_INDEX_MANUFACTURER, // iManufacturer
	STRING_INDEX_PRODUCT, // iProduct
	STRING_INDEX_SERIAL, // iSerialNumber
	4, // bNumConfigurations
};

static const uint8_t STRING_ZERO[4] = {
	sizeof(STRING_ZERO),
	USB_DTYPE_STRING,
	0x09, 0x10, /* English (Canadian) */
};

static const usb_configs_config_t * const CONFIGURATIONS[] = {
	&IDLE_CONFIGURATION,
	&TARGET_CONFIGURATION,
	&ONBOARD_CONFIGURATION,
	&UART_CONFIGURATION,
	0
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
			gpio_push_ep0_cbs();
		} else {
			gpio_remove_ep0_cbs();
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
					case 0: descriptor = IDLE_CONFIGURATION_DESCRIPTOR; break;
					case 1: descriptor = TARGET_CONFIGURATION_DESCRIPTOR; break;
					case 2: descriptor = ONBOARD_CONFIGURATION_DESCRIPTOR; break;
					case 3: descriptor = UART_CONFIGURATION_DESCRIPTOR;
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
						case STRING_INDEX_PRODUCT: string = u8"Flash Memory Burner"; break;
						case STRING_INDEX_CONFIG1: string = u8"Idle/Pre-DFU"; break;
						case STRING_INDEX_CONFIG2: string = u8"Host-Controlled to Target Board"; break;
						case STRING_INDEX_CONFIG3: string = u8"Host-Controlled to Onboard Memory"; break;
						case STRING_INDEX_CONFIG4: string = u8"UART Receiver"; break;
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
	// Shut down the control transfer layer, if the device was already active (implying the control transfer layer had been initialized).
	// As a side effect, this will propagate down the stack to the on_exit handler of any active configuration, ensuring the device properly leaves host-controlled mode if in it.
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

static void handle_usb_unplug(void) {
	// Shut down the control transfer layer, if the device was already active (implying the control transfer layer had been initialized).
	// As a side effect, this will propagate down the stack to the on_exit handler of any active configuration, ensuring the device properly leaves host-controlled mode if in it.
	if (usb_ll_get_state() == USB_LL_STATE_ACTIVE) {
		usb_configs_deinit();
		usb_ep0_deinit();
	}

	// Detach from the bus.
	// We are not allowed to just stay attached to the bus and wait for reset signalling.
	// The reason is that if we are attached to the bus, our pull-up resistor is attached to D+.
	// However, if VBUS is not provided, and we are target-powered, this would back-feed power through D+ from the device to the host.
	// This is illegal: when VBUS is not powered, the pull-up resistor must be detached, so we issue a detach here.
	usb_ll_detach();
	NVIC_ICER[67 / 32] = 1 << (67 % 32); // CLRENA67 = 1; disable USB FS interrupt

	// Maybe there was a race condition or contact bounce and actually VBus is present right now.
	// Just in case that happened, check VBus; if it’s high, reattach.
	if (gpio_get_input(GPIOA, 9)) {
		// VBus detected; initialize USB now.
		usb_ll_attach(&handle_usb_reset, &handle_usb_enumeration_done, &handle_usb_unplug);
		NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt
	}
}

static void handle_usb_plug(void) {
	// Clear the pending interrupt.
	EXTI_PR = 1 << 9;

	// Maybe the USB is not currently detached.
	// In that case, we should do nothing, because one of three cases are possible:
	// (1) This was a small downward dip followed by an upswing in VBus, which was not large enough to cause session end.
	// (2) This was a downward dip in VBus which caused session end and then VBus was restored, but we have not yet processed the session end.
	// (3) This was a small downward dip or contact bounce during plug.
	// In cases 1 and 2, the session end handler (handle_usb_unplug) will detach the USB, then notice that VBus is high and reattach; we need do nothing here.
	// In case 3, the session end handler will never be invoked and we should leave well enough alone and continue waiting for reset signalling.
	if (usb_ll_get_state() != USB_LL_STATE_DETACHED) {
		return;
	}

	// Maybe there was a race condition or contact bounce and actually VBus is not present yet.
	// In that case, do not attach now; we will take another interrupt later and attach then.
	if (!gpio_get_input(GPIOA, 9)) {
		return;
	}

	// Initialize USB now.
	usb_ll_attach(&handle_usb_reset, &handle_usb_enumeration_done, &handle_usb_unplug);
	NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt
}

static void handle_autonomous_burn_and_turn_off(void) {
	// Clear the pending interrupt.
	EXTI_PR = 1 << 15;

	if (usb_ll_get_state() != USB_LL_STATE_ACTIVE || usb_configs_get_current() <= 1) {
		// The USB is not in host-controlled mode.
		if (!autonomous_is_running()) {
			// No autonomous operation is already running.
			// Start one.
			autonomous_start(false);
		}
	}
}

static void handle_autonomous_burn_and_boot(void) {
	// Clear the pending interrupt.
	EXTI_PR = 1 << 14;

	if (usb_ll_get_state() != USB_LL_STATE_ACTIVE || usb_configs_get_current() <= 1) {
		// The USB is not in host-controlled mode.
		if (!autonomous_is_running()) {
			// No autonomous operation is already running.
			// Start one.
			autonomous_start(true);
		}
	}
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

static const exception_app_cbs_t APP_EXCEPTION_CBS = {
	.early = &app_exception_early,
	.late = &app_exception_late,
};

extern unsigned char linker_data_vma_start;
extern unsigned char linker_data_vma_end;
extern const unsigned char linker_data_lma_start;
extern unsigned char linker_bss_vma_start;
extern unsigned char linker_bss_vma_end;

static void stm32_main(void) {
	asm volatile(
			"mov r0, #pstack\n\t"
			"movt r0, #:upper16:pstack\n\t"
			"add r0, #32768\n\t"
			"msr psp, r0\n\t"
			"mov r0, #2\n\t"
			"msr control, r0\n\t"
			"isb\n\t"
			"b main\n\t");
	for (;;);
}

int main(void) {
	// Check if we’re supposed to go to the bootloader.
	RCC_CSR_t rcc_csr_shadow = RCC_CSR; // Keep a copy of RCC_CSR
	RCC_CSR.RMVF = 1; // Clear reset flags
	RCC_CSR.RMVF = 0; // Stop clearing reset flags
	if (rcc_csr_shadow.SFTRSTF && bootload_flag == UINT64_C(0xFE228106195AD2B0)) {
		bootload_flag = 0;
		asm volatile(
			"msr control, %[control]\n\t"
			"isb\n\t"
			"msr msp, %[stack]\n\t"
			"mov pc, %[vector]"
			:
			: [control] "r" (0), [stack] "r" (*(const volatile uint32_t *) 0x1FFF0000), [vector] "r" (*(const volatile uint32_t *) 0x1FFF0004));
	}
	bootload_flag = 0;

	// Copy initialized globals and statics from ROM to RAM.
	memcpy(&linker_data_vma_start, &linker_data_lma_start, &linker_data_vma_end - &linker_data_vma_start);
	// Scrub the BSS section in RAM.
	memset(&linker_bss_vma_start, 0, &linker_bss_vma_end - &linker_bss_vma_start);

	// Always 8-byte-align the stack pointer on entry to an interrupt handler (as ARM recommends).
	CCR.STKALIGN = 1; // Guarantee 8-byte alignment

	// Set the interrupt system to set priorities as having the upper two bits for group priorities and the rest as subpriorities.
	{
		AIRCR_t tmp = AIRCR;
		tmp.VECTKEY = 0x05FA;
		tmp.PRIGROUP = 5;
		AIRCR = tmp;
	}

	// Set up interrupt handling.
	exception_init(&core_progmem_writer, &APP_EXCEPTION_CBS);

	// Set up the memory protection unit to catch bad pointer dereferences.
	// The private peripheral bus (0xE0000000 length 1 MiB) always uses the system memory map, so no region is needed for it.
	// We set up the regions first, then enable the MPU.
	{
		static const struct {
			uint32_t address;
			MPU_RASR_t rasr;
		} REGIONS[] = {
			// 0x08000000–0x080FFFFF (length 1 MiB): Flash memory (normal, read-only, write-through cache, executable)
			{ 0x08000000, { .XN = 0, .AP = 0b111, .TEX = 0b000, .S = 0, .C = 1, .B = 0, .SRD = 0, .SIZE = 19, .ENABLE = 1 } },

			// 0x10000000–0x10007FFF (length 32 kiB): CCM bottom half (pstack) (normal, read-write, write-back write-allocate cache, not executable)
			{ 0x10000000, { .XN = 1, .AP = 0b011, .TEX = 0b001, .S = 0, .C = 1, .B = 1, .SRD = 0, .SIZE = 14, .ENABLE = 1 } },

			// 0x10008000–0x1000FFFF (length 32 kiB): CCM top half (mstack) (normal, read-write, write-back write-allocate cache, not executable, privileged-only):
			{ 0x10008000, { .XN = 1, .AP = 0b001, .TEX = 0b001, .S = 0, .C = 1, .B = 1, .SRD = 0, .SIZE = 14, .ENABLE = 1 } },

			// 0x1FFF0000–0x1FFF7FFF (length 32 kiB): System memory including U_ID and F_SIZE (normal, read-only, write-through cache, not executable)
			{ 0x1FFF0000, { .XN = 1, .AP = 0b111, .TEX = 0b000, .S = 0, .C = 1, .B = 0, .SRD = 0, .SIZE = 14, .ENABLE = 1 } },

			// 0x20000000–0x2001FFFF (length 128 kiB): SRAM (normal, read-write, write-back write-allocate cache, not executable)
			{ 0x20000000, { .XN = 1, .AP = 0b011, .TEX = 0b001, .S = 0, .C = 1, .B = 1, .SRD = 0, .SIZE = 16, .ENABLE = 1 } },

			// 0x40000000–0x4007FFFF (length 512 kiB): Peripherals (device, read-write, not executable) using subregions:
			// Subregion 0 (0x40000000–0x4000FFFF): Enabled (contains APB1)
			// Subregion 1 (0x40010000–0x4001FFFF): Enabled (contains APB2)
			// Subregion 2 (0x40020000–0x4002FFFF): Enabled (contains AHB1)
			// Subregion 3 (0x40030000–0x4003FFFF): Disabled
			// Subregion 4 (0x40040000–0x4004FFFF): Disabled
			// Subregion 5 (0x40050000–0x4005FFFF): Disabled
			// Subregion 6 (0x40060000–0x4006FFFF): Disabled
			// Subregion 7 (0x40070000–0x4007FFFF): Disabled
			{ 0x40000000, { .XN = 1, .AP = 0b011, .TEX = 0b010, .S = 0, .C = 0, .B = 0, .SRD = 0b11111000, .SIZE = 18, .ENABLE = 1 } },

			// 0x50000000–0x5007FFFF (length 512 kiB): Peripherals (device, read-write, not executable) using subregions:
			// Subregion 0 (0x50000000–0x5000FFFF): Enabled (contains AHB2)
			// Subregion 1 (0x50010000–0x5001FFFF): Enabled (contains AHB2)
			// Subregion 2 (0x50020000–0x5002FFFF): Enabled (contains AHB2)
			// Subregion 3 (0x50030000–0x5003FFFF): Enabled (contains AHB2)
			// Subregion 4 (0x50040000–0x5004FFFF): Enabled (contains AHB2)
			// Subregion 5 (0x50050000–0x5005FFFF): Enabled (contains AHB2)
			// Subregion 6 (0x50060000–0x5006FFFF): Enabled (contains AHB2)
			// Subregion 7 (0x50070000–0x5007FFFF): Disabled
			{ 0x50000000, { .XN = 1, .AP = 0b011, .TEX = 0b010, .S = 0, .C = 0, .B = 0, .SRD = 0b10000000, .SIZE = 18, .ENABLE = 1 } },
		};
		for (unsigned int i = 0; i < sizeof(REGIONS) / sizeof(*REGIONS); ++i) {
			MPU_RNR_t rnr = { .REGION = i };
			MPU_RNR = rnr;
			MPU_RBAR.ADDR = REGIONS[i].address >> 5;
			MPU_RASR = REGIONS[i].rasr;
		}
	}
	{
		MPU_CTRL_t tmp = {
			.PRIVDEFENA = 0, // Background region is disabled even in privileged mode.
			.HFNMIENA = 0, // Protection unit disables itself when taking hard faults, memory faults, and NMIs.
			.ENABLE = 1, // Enable MPU.
		};
		MPU_CTRL = tmp;
	}
	asm volatile("dsb");
	asm volatile("isb");

	// Enable the SYSCFG module.
	rcc_enable(APB2, SYSCFG);
	rcc_reset(APB2, SYSCFG);

	// Enable the I/O compensation cell because we will be using some GPIOs at 50+ MHz.
	{
		SYSCFG_CMPCR_t tmp = { .CMP_PD = 1 };
		SYSCFG_CMPCR = tmp;
	}
	while (!SYSCFG_CMPCR.READY);

	// Enable the HSE (8 MHz crystal) oscillator.
	{
		RCC_CR_t tmp = {
			.PLLI2SON = 0, // I²S PLL off.
			.PLLON = 0, // Main PLL off.
			.CSSON = 0, // Clock security system off.
			.HSEBYP = 0, // HSE oscillator in circuit.
			.HSEON = 1, // HSE oscillator enabled.
			.HSITRIM = 16, // HSI oscillator trimmed to midpoint.
			.HSION = 1, // HSI oscillator enabled (still using it at this point).
		};
		RCC_CR = tmp;
	}
	// Wait for the HSE oscillator to be ready.
	while (!RCC_CR.HSERDY);
	// Configure the PLL.
	{
		RCC_PLLCFGR_t tmp = {
			.PLLQ = 7, // Divide 336 MHz VCO output by 7 to get 48 MHz USB, SDIO, and RNG clock
			.PLLSRC = 1, // Use HSE for PLL input
			.PLLP = 0, // Divide 336 MHz VCO output by 2 to get 168 MHz SYSCLK
			.PLLN = 168, // Multiply 2 MHz VCO input by 168 to get 336 MHz VCO output
			.PLLM = 4, // Divide 8 MHz HSE by 4 to get 2 MHz VCO input
		};
		RCC_PLLCFGR = tmp;
	}
	// Enable the PLL.
	RCC_CR.PLLON = 1; // Enable PLL
	// Wait for the PLL to lock.
	while (!RCC_CR.PLLRDY);
	// Set up bus frequencies.
	{
		RCC_CFGR_t tmp = {
			.MCO2 = 2, // MCO2 pin outputs HSE
			.MCO2PRE = 0, // Divide 8 MHz HSE by 1 to get 8 MHz MCO2 (must be ≤ 100 MHz)
			.MCO1PRE = 0, // Divide 8 MHz HSE by 1 to get 8 MHz MCO1 (must be ≤ 100 MHz)
			.I2SSRC = 0, // I²S module gets clock from PLLI2X
			.MCO1 = 2, // MCO1 pin outputs HSE
			.RTCPRE = 8, // Divide 8 MHz HSE by 8 to get 1 MHz RTC clock (must be 1 MHz)
			.PPRE2 = 4, // Divide 144 MHz AHB clock by 2 to get 72 MHz APB2 clock (must be ≤ 84 MHz)
			.PPRE1 = 5, // Divide 144 MHz AHB clock by 4 to get 36 MHz APB1 clock (must be ≤ 42 MHz)
			.HPRE = 0, // Divide 144 MHz SYSCLK by 1 to get 144 MHz AHB clock (must be ≤ 168 MHz)
			.SW = 0, // Use HSI for SYSCLK for now, until everything else is ready
		};
		RCC_CFGR = tmp;
	}
	// Wait 16 AHB cycles for the new prescalers to settle.
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	// Set Flash access latency to 4 wait states.
	{
		FLASH_ACR_t tmp = {
			.DCRST = 0, // Do not clear data cache at this time.
			.ICRST = 0, // Do not clear instruction cache at this time.
			.DCEN = 0, // Do not enable data cache at this time.
			.ICEN = 0, // Do not enable instruction cache at this time.
			.PRFTEN = 0, // Do not enable prefetcher at this time.
			.LATENCY = 5, // Five wait states (acceptable for 150 ≤ HCLK ≤ 168)
		};
		FLASH_ACR = tmp;
	}
	// Flash access latency change may not be immediately effective; wait until it’s locked in.
	while (FLASH_ACR.LATENCY != 5);
	// Actually initiate the clock switch.
	RCC_CFGR.SW = 2; // Use PLL for SYSCLK
	// Wait for the clock switch to complete.
	while (RCC_CFGR.SWS != 2);
	// Turn off the HSI now that it’s no longer needed.
	RCC_CR.HSION = 0; // Disable HSI

	// Flush any data in the CPU caches (which are not presently enabled).
	{
		FLASH_ACR_t tmp = FLASH_ACR;
		tmp.DCRST = 1; // Reset data cache.
		tmp.ICRST = 1; // Reset instruction cache.
		FLASH_ACR = tmp;
	}
	{
		FLASH_ACR_t tmp = FLASH_ACR;
		tmp.DCRST = 0; // Stop resetting data cache.
		tmp.ICRST = 0; // Stop resetting instruction cache.
		FLASH_ACR = tmp;
	}

	// Turn on the caches.
	// There is an errata that says prefetching doesn’t work on some silicon, but it seems harmless to enable the flag even so.
	{
		FLASH_ACR_t tmp = FLASH_ACR;
		tmp.DCEN = 1; // Enable data cache
		tmp.ICEN = 1; // Enable instruction cache
		tmp.PRFTEN = 1; // Enable prefetching
		FLASH_ACR = tmp;
	}

	// Set SYSTICK to divide by 168 so it overflows every microsecond.
	SYST_RVR.RELOAD = 168 - 1;
	// Set SYSTICK to run with the core AHB clock.
	{
		SYST_CSR_t tmp = {
			.CLKSOURCE = 1, // Use core clock
			.ENABLE = 1, // Counter is running
		};
		SYST_CSR = tmp;
	}
	// Reset the counter.
	SYST_CVR.CURRENT = 0;

	// Set up pins.
	rcc_enable(AHB1, GPIOA);
	rcc_enable(AHB1, GPIOB);
	rcc_enable(AHB1, GPIOC);
	rcc_enable(AHB1, GPIOD);
	rcc_reset(AHB1, GPIOA);
	rcc_reset(AHB1, GPIOB);
	rcc_reset(AHB1, GPIOC);
	rcc_reset(AHB1, GPIOD);
	// PA15 = external Flash /CS, input with no resistors until needed
	// PA14 = PROGRAM_B, input with no resistors until needed
	// PA13 = N/C, driven low
	// PA12/PA11 = alternate function OTG FS
	// PA10 = N/C, driven low
	// PA9 = OTG FS VBUS, input with weak pull-down
	// PA8/PA7/PA6 = N/C
	// PA5 = shorted to VDD, driven high
	// PA4 = internal Flash /CS, start deasserted (high)
	// PA3 = shorted to VSS, driven low
	// PA2 = N/C, driven low
	// PA1/PA0 = shorted to VDD, driven high
	GPIOA.ODR = 0b0000000000110011;
	GPIOA.OTYPER = 0b0000000000000000;
	GPIOA.OSPEEDR = 0b01000001010000000000000100000000;
	GPIOA.PUPDR = 0b00000000000010000000000000000000;
	GPIOA.AFRH = 0b00000000000010101010000000000000;
	GPIOA.AFRL = 0b00000000000000000000000000000000;
	GPIOA.MODER = 0b00000110100100010101010101010101;
	// PB15 = N/C, driven low
	// PB14 = LED 3, start high (on)
	// PB13 = LED 2, start high (on)
	// PB12 = LED 1, start high (on)
	// PB11/PB10 = N/C, driven low
	// PB9/PB8 = shorted to VSS, driven low
	// PB7 = alternate function USART 1 receive
	// PB6 = N/C, driven low
	// PB5 = external Flash MOSI, input with no resistors until needed
	// PB4 = external Flash MISO, input with no resistors until needed
	// PB3 = external Flash SCK, input with no resistors until needed
	// PB2 = BOOT1, hardwired low
	// PB1/PB0 = N/C, driven low
	GPIOB.ODR = 0b0111000000000000;
	GPIOB.OTYPER = 0b0000000000000000;
	GPIOB.OSPEEDR = 0b00000000000000000000100010000000;
	GPIOB.PUPDR = 0b00000000000000000000000000000000;
	GPIOB.AFRH = 0b00000000000000000000000000000000;
	GPIOB.AFRL = 0b01110000010101010101000000000000;
	GPIOB.MODER = 0b01010101010101011001000000010101;
	// PC15 = switch SW2 to ground (input with pull-up)
	// PC14 = switch SW3 to ground (input with pull-up)
	// PC13 = N/C, driven low
	// PC12 = alternate function internal Flash (SPI3) MOSI
	// PC11 = alternate function internal Flash (SPI3) MISO
	// PC10 = alternate function internal Flash (SPI3) SCK
	// PC9/PC8/PC7/PC6/PC5/PC4/PC3/PC2/PC1/PC0 = N/C, driven low
	GPIOC.ODR = 0b0000000000000000;
	GPIOC.OTYPER = 0b0000000000000000;
	GPIOC.OSPEEDR = 0b00000010101000000000000000000000;
	GPIOC.PUPDR = 0b01010000000000000000000000000000;
	GPIOC.AFRH = 0b00000000000001100110011000000000;
	GPIOC.AFRL = 0b00000000000000000000000000000000;
	GPIOC.MODER = 0b00000110101001010101010101010101;
	// PD15/PD14/PD13/PD12/PD11/PD10/PD9/PD8/PD7/PD6/PD5/PD4/PD3 = unimplemented on package
	// PD2 = external power control, input with no resistors until needed
	// PD1/PD0 = unimplemented on package
	GPIOD.ODR = 0b0000000000000000;
	GPIOD.OTYPER = 0b0000000000000000;
	GPIOD.OSPEEDR = 0b00000000000000000000000000000000;
	GPIOD.PUPDR = 0b00000000000000000000000000000000;
	GPIOD.AFRH = 0b00000000000000000000000000000000;
	GPIOD.AFRL = 0b00000000000000000000000000000000;
	GPIOD.MODER = 0b01010101010101010101010101000101;
	// PE/PF/PG = unimplemented on this package
	// PH15/PH14/PH13/PH12/PH11/PH10/PH9/PH8/PH7/PH6/PH5/PH4/PH3/PH2 = unimplemented on this package
	// PH1 = OSC_OUT (not configured via GPIO registers)
	// PH0 = OSC_IN (not configured via GPIO registers)
	// PI15/PI14/PI13/PI12 = unimplemented
	// PI11/PI10/PI9/PI8/PI7/PI6/PI5/PI4/PI3/PI2/PI1/PI0 = unimplemented on this package

	// Wait a bit.
	sleep_ms(100);

	// Turn off LEDs 2 and 3.
	gpio_set_reset_mask(GPIOB, 0, 3 << 13);

	// Enable the SPI transceiver modules.
	spi_init();

	// Determine whether to enable USB now depending on the state of VBus.
	if (gpio_get_input(GPIOA, 9)) {
		// VBus detected; initialize USB now.
		usb_ll_attach(&handle_usb_reset, &handle_usb_enumeration_done, &handle_usb_unplug);
		NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt
	}

	// Enable an interrupt to handle VBus going high when the USB cable is plugged in.
	exti_map(9, 0);
	exti_set_handler(9, &handle_usb_plug);
	EXTI_RTSR |= 1 << 9;
	EXTI_IMR |= 1 << 9;
	NVIC_ISER[23 / 32] = 1 << (23 % 32); // SETENA23 = 1; enable EXTI 5 through 9 interrupts

	// Enable an interrupt to handle an autonomous mode pushbutton being pushed.
	// PC15 is SW2 which is burn and turn off.
	// PC14 is SW3 which is burn and boot.
	exti_map(14, 2);
	exti_set_handler(14, &handle_autonomous_burn_and_boot);
	exti_map(15, 2);
	exti_set_handler(15, &handle_autonomous_burn_and_turn_off);
	EXTI_FTSR |= (1 << 14) | (1 << 15);
	EXTI_IMR |= (1 << 14) | (1 << 15);
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI 15 through 10 interrupts

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

