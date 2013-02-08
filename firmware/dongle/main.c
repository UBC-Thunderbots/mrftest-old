#include "buzzer.h"
#include "configs.h"
#include "constants.h"
#include "estop.h"
#include "format.h"
#include "mrf.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "unused.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

static void stm32_main(void) __attribute__((noreturn));
static void nmi_vector(void);
static void hard_fault_vector(void);
static void memory_manage_vector(void);
static void bus_fault_vector(void);
static void usage_fault_vector(void);
static void service_call_vector(void);
static void pending_service_vector(void);
static void system_tick_vector(void);
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

static char stack[65536] __attribute__((section(".stack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16] __attribute__((used, section(".exception_vectors"))) = {
	[0] = (fptr) (stack + sizeof(stack)),
	[1] = &stm32_main,
	[2] = &nmi_vector,
	[3] = &hard_fault_vector,
	[4] = &memory_manage_vector,
	[5] = &bus_fault_vector,
	[6] = &usage_fault_vector,
	[11] = &service_call_vector,
	[14] = &pending_service_vector,
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
	[67] = &usb_process,
};

static void nmi_vector(void) {
	for (;;);
}

static void hard_fault_vector(void) {
	for (;;);
}

static void memory_manage_vector(void) {
	for (;;);
}

static void bus_fault_vector(void) {
	for (;;);
}

static void usage_fault_vector(void) {
	for (;;);
}

static void service_call_vector(void) {
	for (;;);
}

static void pending_service_vector(void) {
	for (;;);
}

static void system_tick_vector(void) {
	for (;;);
}

volatile uint64_t bootload_flag;

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept, usb_ep0_poststatus_callback_t *UNUSED(poststatus)) {
	if (request_type == (USB_STD_REQ_TYPE_VENDOR | USB_STD_REQ_TYPE_DEVICE) && request == CONTROL_REQUEST_BEEP && !index) {
		buzzer_start(value);
		*accept = true;
		return true;
	}
	return false;
}

static bool on_in_request(uint8_t UNUSED(request_type), uint8_t UNUSED(request), uint16_t UNUSED(value), uint16_t UNUSED(index), uint16_t UNUSED(length), usb_ep0_source_t **UNUSED(source), usb_ep0_poststatus_callback_t *UNUSED(poststatus)) {
	return false;
}

static bool on_out_request(uint8_t UNUSED(request_type), uint8_t UNUSED(request), uint16_t UNUSED(value), uint16_t UNUSED(index), uint16_t UNUSED(length), void **UNUSED(dest), bool (**UNUSED_cb)(void) __attribute__((unused)), usb_ep0_poststatus_callback_t *UNUSED(poststatus)) {
	return false;
}

static const uint8_t DEVICE_DESCRIPTOR[18] = {
	18, // bLength
	1, // bDescriptorType
	0, // bcdUSB LSB
	2, // bcdUSB MSB
	0xFF, // bDeviceClass
	0, // bDeviceSubClass
	0, // bDeviceProtocol
	8, // bMaxPacketSize0
	0x83, // idVendor LSB
	0x04, // idVendor MSB
	0x7C, // idProduct LSB
	0x49, // idProduct MSB
	0, // bcdDevice LSB
	1, // bcdDevice MSB
	STRING_INDEX_MANUFACTURER, // iManufacturer
	STRING_INDEX_PRODUCT, // iProduct
	STRING_INDEX_SERIAL, // iSerialNumber
	6, // bNumConfigurations
};

static const uint8_t CONFIGURATION_DESCRIPTOR4[] = {
	9, // bLength
	2, // bDescriptorType
	18, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	4, // bConfigurationValue
	STRING_INDEX_CONFIG4, // iConfiguration
	0x80, // bmAttributes
	150, // bMaxPower

	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	0, // bNumEndpoints
	0xFF, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0, // bInterfaceProtocol
	0, // iInterface
};

static const uint8_t CONFIGURATION_DESCRIPTOR5[] = {
	9, // bLength
	2, // bDescriptorType
	25, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	5, // bConfigurationValue
	STRING_INDEX_CONFIG5, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

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
	20, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval
};

static const uint8_t * const CONFIGURATION_DESCRIPTORS[] = {
	CONFIGURATION_DESCRIPTOR1,
	CONFIGURATION_DESCRIPTOR2,
	CONFIGURATION_DESCRIPTOR3,
	CONFIGURATION_DESCRIPTOR4,
	CONFIGURATION_DESCRIPTOR5,
	CONFIGURATION_DESCRIPTOR6,
};

static const uint8_t STRING_ZERO[4] = {
	sizeof(STRING_ZERO),
	USB_STD_DESCRIPTOR_STRING,
	0x09, 0x10, /* English (Canadian) */
};

static usb_ep0_source_t *on_descriptor_request(uint8_t descriptor_type, uint8_t descriptor_index, uint16_t language) {
	static union {
		usb_ep0_memory_source_t mem_src;
		usb_ep0_string_descriptor_source_t string_src;
	} src;
	static char serial_number_buffer[25];

	if (descriptor_type == USB_STD_DESCRIPTOR_DEVICE) {
		return usb_ep0_memory_source_init(&src.mem_src, DEVICE_DESCRIPTOR, sizeof(DEVICE_DESCRIPTOR));
	} else if (descriptor_type == USB_STD_DESCRIPTOR_CONFIGURATION) {
		if (descriptor_index < 6) {
			const uint8_t *desc = CONFIGURATION_DESCRIPTORS[descriptor_index];
			return usb_ep0_memory_source_init(&src.mem_src, desc, (desc[3] << 8) | desc[2]);
		}
	} else if (descriptor_type == USB_STD_DESCRIPTOR_STRING) {
		if (descriptor_index == 0) {
			return usb_ep0_memory_source_init(&src.mem_src, STRING_ZERO, sizeof(STRING_ZERO));
		} else if (language == 0x1009 /* English (Canadian) */) {
			const char *string = 0;
			switch (descriptor_index) {
				case STRING_INDEX_MANUFACTURER: string = u8"UBC Thunderbots Small Size Team"; break;
				case STRING_INDEX_PRODUCT: string = u8"Radio Base Station"; break;
				case STRING_INDEX_CONFIG1: string = u8"Radio Sleep/Pre-DFU"; break;
				case STRING_INDEX_CONFIG2: string = u8"Normal Operation"; break;
				case STRING_INDEX_CONFIG3: string = u8"Promiscuous Mode"; break;
				case STRING_INDEX_CONFIG4: string = u8"Packet Generator"; break;
				case STRING_INDEX_CONFIG5: string = u8"Packet Receiver"; break;
				case STRING_INDEX_CONFIG6: string = u8"Debug Mode"; break;
				case STRING_INDEX_SERIAL:
					formathex32(serial_number_buffer + 0, U_ID_H);
					formathex32(serial_number_buffer + 8, U_ID_M);
					formathex32(serial_number_buffer + 16, U_ID_L);
					serial_number_buffer[24] = 0;
					string = serial_number_buffer;
					break;
			}
			if (string) {
				return usb_ep0_string_descriptor_source_init(&src.string_src, string);
			} else {
				return 0;
			}
		}
	}

	return 0;
}

static bool on_check_self_powered(void) {
	return false;
}

static const usb_ep0_global_callbacks_t DEVICE_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
	.on_out_request = &on_out_request,
	.on_descriptor_request = &on_descriptor_request,
	.on_check_self_powered = &on_check_self_powered,
};

static bool can_enter_config4(void) {
#warning TODO implement this configuration
	return false;
}

static void on_enter_config4(void) {
}

static void on_exit_config4(void) {
}

static bool can_enter_config5(void) {
#warning TODO implement this configuration
	return false;
}

static void on_enter_config5(void) {
}

static void on_exit_config5(void) {
}

static void handle_usb_reset(void) {
	buzzer_stop();
}

static const usb_ep0_configuration_callbacks_t CONFIG4_CBS = {
	.configuration = 4,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 0,
	.can_enter = &can_enter_config4,
	.on_enter = &on_enter_config4,
	.on_exit = &on_exit_config4,
	.on_zero_request = 0,
	.on_in_request = 0,
	.on_out_request = 0,
};

static const usb_ep0_configuration_callbacks_t CONFIG5_CBS = {
	.configuration = 5,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 0,
	.can_enter = &can_enter_config5,
	.on_enter = &on_enter_config5,
	.on_exit = &on_exit_config5,
	.on_zero_request = 0,
	.on_in_request = 0,
	.on_out_request = 0,
};

static const usb_ep0_configuration_callbacks_t * const CONFIG_CBS[] = {
	&CONFIGURATION_CBS1,
	&CONFIGURATION_CBS2,
	&CONFIGURATION_CBS3,
	&CONFIG4_CBS,
	&CONFIG5_CBS,
	&CONFIGURATION_CBS6,
	0
};

static const usb_device_info_t DEVICE_INFO = {
	.rx_fifo_words = 128,
	.ep0_max_packet = 8,
	.on_reset = &handle_usb_reset,
};

extern unsigned char linker_data_vma_start;
extern unsigned char linker_data_vma_end;
extern const unsigned char linker_data_lma_start;
extern unsigned char linker_bss_vma_start;
extern unsigned char linker_bss_vma_end;

static void stm32_main(void) {
	// Check if we’re supposed to go to the bootloader.
	uint32_t rcc_csr_shadow = RCC_CSR; // Keep a copy of RCC_CSR
	RCC_CSR |= RMVF; // Clear reset flags
	RCC_CSR &= ~RMVF; // Stop clearing reset flags
	if ((rcc_csr_shadow & SFTRSTF) && bootload_flag == UINT64_C(0xDEADBEEFCAFEBABE)) {
		bootload_flag = 0;
		asm volatile(
			"mov sp, %[stack]\n\t"
			"mov pc, %[vector]"
			:
			: [stack] "r" (*(const volatile uint32_t *) 0x1FFF0000), [vector] "r" (*(const volatile uint32_t *) 0x1FFF0004));
	}
	bootload_flag = 0;

	// Copy initialized globals and statics from ROM to RAM.
	memcpy(&linker_data_vma_start, &linker_data_lma_start, &linker_data_vma_end - &linker_data_vma_start);
	// Scrub the BSS section in RAM.
	memset(&linker_bss_vma_start, 0, &linker_bss_vma_end - &linker_bss_vma_start);

	// Always 8-byte-align the stack pointer on entry to an interrupt handler (as ARM recommends).
	SCS_CCR |= STKALIGN; // Guarantee 8-byte alignment

	// Enable the HSE (8 MHz crystal) oscillator.
	RCC_CR = HSEON // Enable HSE oscillator
		| HSITRIM(16) // Trim HSI oscillator to midpoint
		| HSION; // Enable HSI oscillator for now as we’re still using it
	// Wait for the HSE oscillator to be ready.
	while (!(RCC_CR & HSERDY));
	// Configure the PLL.
	RCC_PLLCFGR = PLLQ(6) // Divide 288 MHz VCO output by 6 to get 48 MHz USB, SDIO, and RNG clock
		| PLLSRC // Use HSE for PLL input
		| PLLP(0) // Divide 288 MHz VCO output by 2 to get 144 MHz SYSCLK
		| PLLN(144) // Multiply 2 MHz VCO input by 144 to get 288 MHz VCO output
		| PLLM(4); // Divide 8 MHz HSE by 4 to get 2 MHz VCO input
	// Enable the PLL.
	RCC_CR |= PLLON; // Enable PLL
	// Wait for the PLL to lock.
	while (!(RCC_CR & PLLRDY));
	// Set up bus frequencies.
	RCC_CFGR = MCO2(2) // MCO2 pin outputs HSE
		| MCO2PRE(0) // Divide 8 MHz HSE by 1 to get 8 MHz MCO2 (must be ≤ 100 MHz)
		| MCO1PRE(0) // Divide 8 MHz HSE by 1 to get 8 MHz MCO1 (must be ≤ 100 MHz)
		| 0 // I2SSRC = 0; I2S module gets clock from PLLI2X
		| MCO1(2) // MCO1 pin outputs HSE
		| RTCPRE(8) // Divide 8 MHz HSE by 8 to get 1 MHz RTC clock (must be 1 MHz)
		| PPRE2(4) // Divide 144 MHz AHB clock by 2 to get 72 MHz APB2 clock (must be ≤ 84 MHz)
		| PPRE1(5) // Divide 144 MHz AHB clock by 4 to get 36 MHz APB1 clock (must be ≤ 42 MHz)
		| HPRE(0) // Divide 144 MHz SYSCLK by 1 to get 144 MHz AHB clock (must be ≤ 168 MHz)
		| SW(0); // Use HSI for SYSCLK for now, until everything else is ready
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
	// Set Flash access latency to 5 wait states.
	FLASH_ACR = LATENCY(4); // Four wait states (acceptable for 120 ≤ HCLK ≤ 150)
	// Flash access latency change may not be immediately effective; wait until it’s locked in.
	while (LATENCY_X(FLASH_ACR) != 4);
	// Actually initiate the clock switch.
	RCC_CFGR = (RCC_CFGR & ~SW_MSK) | SW(2); // Use PLL for SYSCLK
	// Wait for the clock switch to complete.
	while (SWS_X(RCC_CFGR) != 2);
	// Turn off the HSI now that it’s no longer needed.
	RCC_CR &= ~HSION; // Disable HSI

	// Flush any data in the CPU caches (which are not presently enabled).
	FLASH_ACR |= DCRST // Reset data cache
		| ICRST; // Reset instruction cache
	FLASH_ACR &= ~DCRST // Stop resetting data cache
		& ~ICRST; // Stop resetting instruction cache

	// Turn on the caches.
	// There is an errata that says prefetching doesn’t work on some silicon, but it seems harmless to enable the flag even so.
	FLASH_ACR |= DCEN // Enable data cache
		| ICEN // Enable instruction cache
		| PRFTEN; // Enable prefetching

	// Enable the system configuration registers.
	rcc_enable(APB2, 14);

	// Set SYSTICK to divide by 144 so it overflows every microsecond.
	SCS_STRVR = 144 - 1;
	// Set SYSTICK to run with the core AHB clock.
	SCS_STCSR = CLKSOURCE // Use core clock
		| SCS_STCSR_ENABLE; // Counter is running
	// Reset the counter.
	SCS_STCVR = 0;

	// As we will be running at 144 MHz, switch to the lower-power voltage regulator mode (compatible only up to 144 MHz).
	rcc_enable(APB1, 28);
	PWR_CR &= ~VOS; // Set regulator scale 2
	rcc_disable(APB1, 28);

	// Initialize subsystems.
	buzzer_init();

	// Set up pins.
	rcc_enable_multi(AHB1, 0x0000000F); // Enable GPIOA, GPIOB, GPIOC, and GPIOD modules
	// PA15 = MRF /CS, start deasserted
	// PA14/PA13 = alternate function SWD
	// PA12/PA11 = alternate function OTG FS
	// PA10/PA9/PA8/PA7/PA6 = N/C
	// PA5/PA4 = shorted to VDD
	// PA3 = shorted to VSS
	// PA2 = alternate function TIM2 buzzer
	// PA1/PA0 = shorted to VDD
	GPIOA_ODR = 0b1000000000110011;
	GPIOA_OSPEEDR = 0b01000011110000000000000000000000;
	GPIOA_PUPDR = 0b00100100000000000000000000000000;
	GPIOA_AFRH = 0b00000000000010101010000000000000;
	GPIOA_AFRL = 0b00000000000000000000000100000000;
	GPIOA_MODER = 0b01101010100101010101010101100101;
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
	GPIOB_ODR = 0b0111000000000000;
	GPIOB_OSPEEDR = 0b00000000000000000000010001000000;
	GPIOB_PUPDR = 0b00000000000000000000001000000000;
	GPIOB_AFRH = 0b00000000000000000000000000000000;
	GPIOB_AFRL = 0b00000000010101010101000000000000;
	GPIOB_MODER = 0b01010101010101010101101010011101;
	// PC15/PC14/PC13 = N/C
	// PC12 = MRF INT, input
	// PC11/PC10/PC9/PC8/PC7/PC6 = N/C
	// PC5 = run switch negative supply, always low
	// PC4/PC3/PC2/PC1/PC0 = N/C
	GPIOC_ODR = 0b0000000000000000;
	GPIOC_OSPEEDR = 0b00000000000000000000000000000000;
	GPIOC_PUPDR = 0b00000000000000000000000000000000;
	GPIOC_AFRH = 0b00000000000000000000000000000000;
	GPIOC_AFRL = 0b00000000000000000000000000000000;
	GPIOC_MODER = 0b01010100010101010101010101010101;
	// PD15/PD14/PD13/PD12/PD11/PD10/PD9/PD8/PD7/PD6/PD5/PD4/PD3 = unimplemented on package
	// PD2 = N/C
	// PD1/PD0 = unimplemented on package
	GPIOD_ODR = 0b0000000000000000;
	GPIOD_OSPEEDR = 0b00000000000000000000000000000000;
	GPIOD_PUPDR = 0b00000000000000000000000000000000;
	GPIOD_AFRH = 0b00000000000000000000000000000000;
	GPIOD_AFRL = 0b00000000000000000000000000000000;
	GPIOD_MODER = 0b01010101010101010101010101010101;
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
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Initialize USB.
	usb_ep0_set_global_callbacks(&DEVICE_CBS);
	usb_ep0_set_configuration_callbacks(CONFIG_CBS);
	usb_attach(&DEVICE_INFO);
	NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt

	// Handle activity.
	while (usb_get_device_state() != USB_DEVICE_STATE_DETACHED);

	// Reboot.
	SCS_AIRCR = VECTKEY(0x05FA) | SYSRESETREQ;
	for (;;);
}

