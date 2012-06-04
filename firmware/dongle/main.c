#include "buzzer.h"
#include "mrf.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
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
extern void timer5_interrupt_vector(void);

static char stack[65536] __attribute__((section(".stack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16] __attribute__((used, section(".exception_vectors"))) = {
	// Vector 0 contains the reset stack pointer
	[0] = (fptr) (stack + sizeof(stack)),
	// Vector 1 contains the reset vector
	[1] = &stm32_main,
	// Vector 2 contains the NMI vector
	[2] = &nmi_vector,
	// Vector 3 contains the HardFault vector
	[3] = &hard_fault_vector,
	// Vector 4 contains the MemManage vector
	[4] = &memory_manage_vector,
	// Vector 5 contains the BusFault vector
	[5] = &bus_fault_vector,
	// Vector 6 contains the UsageFault vector
	[6] = &usage_fault_vector,
	// Vector 11 contains the SVCall vector
	[11] = &service_call_vector,
	// Vector 14 contains the PendSV vector
	[14] = &pending_service_vector,
	// Vector 15 contains the SysTick vector
	[15] = &system_tick_vector,
};

static const fptr interrupt_vectors[82] __attribute__((used, section(".interrupt_vectors"))) = {
	// Vector 50 contains the timer 5 vector
	[50] = &timer5_interrupt_vector,
	// Vector 67 contains the USB full speed vector
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

static volatile uint64_t bootload_flag;

static bool on_zero_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, bool *accept) {
	if (request_type == 0x40 && request == 0x12 && !index) {
		buzzer_start(value);
		*accept = true;
		return true;
	} else if (request_type == 0x40 && request == 0x13 && !value && !index && usb_ep0_get_configuration() == 0) {
		bootload_flag = UINT64_C(0xDEADBEEFCAFEBABE);
		SCS_AIRCR = 0x05FA0004;
		for (;;);
	}
	return false;
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, usb_ep0_source_t **source) {
	return false;
}

static bool on_out_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void **dest, bool (**cb)(void)) {
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
	0x57, // idVendor LSB
	0xC0, // idVendor MSB
	0x79, // idProduct LSB
	0x25, // idProduct MSB
	0, // bcdDevice LSB
	1, // bcdDevice MSB
	1, // iManufacturer
	2, // iProduct
	9, // iSerialNumber
	6, // bNumConfigurations
};

static const uint8_t CONFIGURATION_DESCRIPTOR1[] = {
	9, // bLength
	2, // bDescriptorType
	18, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	1, // bConfigurationValue
	3, // iConfiguration
	0x80, // bmAttributes
	50, // bMaxPower

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

static const uint8_t CONFIGURATION_DESCRIPTOR2[] = {
	9, // bLength
	2, // bDescriptorType
	60, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	2, // bConfigurationValue
	4, // iConfiguration
	0x80, // bmAttributes
	150, // bMaxPower

	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	6, // bNumEndpoints
	0xFF, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	5, // bDescriptorType
	0x01, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x02, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x03, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x81, // bEndpointAddress
	0x03, // bmAttributes
	2, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x82, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x83, // bEndpointAddress
	0x03, // bmAttributes
	2, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	1, // bInterval
};

static const uint8_t CONFIGURATION_DESCRIPTOR3[] = {
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

static const uint8_t CONFIGURATION_DESCRIPTOR4[] = {
	9, // bLength
	2, // bDescriptorType
	18, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	4, // bConfigurationValue
	6, // iConfiguration
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
	7, // iConfiguration
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

static const uint8_t CONFIGURATION_DESCRIPTOR6[] = {
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
				case 1: string = u8"UBC Thunderbots Small Size Team"; break;
				case 2: string = u8"Radio Base Station"; break;
				case 3: string = u8"Radio Sleep"; break;
				case 4: string = u8"Normal Operation"; break;
				case 5: string = u8"Promiscuous Mode"; break;
				case 6: string = u8"Packet Generator"; break;
				case 7: string = u8"Packet Receiver"; break;
				case 8: string = u8"Debug Mode"; break;
				case 9:
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

static void on_enter_config1(void) {
	GPIOB_ODR |= 7 << 12;
}

static bool can_enter_config2(void) {
#warning TODO implement this configuration
	return false;
}

static void on_enter_config2(void) {
}

static void on_exit_config2(void) {
}

static bool can_enter_config3(void) {
#warning TODO implement this configuration
	return false;
}

static void on_enter_config3(void) {
}

static void on_exit_config3(void) {
}

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

static bool can_enter_config6(void) {
#warning TODO implement this configuration
	return false;
}

static void on_enter_config6(void) {
}

static void on_exit_config6(void) {
}

static const usb_ep0_configuration_callbacks_t CONFIG1_CBS = {
	.configuration = 1,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 0,
	.can_enter = 0,
	.on_enter = &on_enter_config1,
	.on_exit = 0,
	.on_zero_request = 0,
	.on_in_request = 0,
	.on_out_request = 0,
};

static const usb_ep0_configuration_callbacks_t CONFIG2_CBS = {
	.configuration = 2,
	.interfaces = 1,
	.out_endpoints = 3,
	.in_endpoints = 3,
	.can_enter = &can_enter_config2,
	.on_enter = &on_enter_config2,
	.on_exit = &on_exit_config2,
	.on_zero_request = 0,
	.on_in_request = 0,
	.on_out_request = 0,
};

static const usb_ep0_configuration_callbacks_t CONFIG3_CBS = {
	.configuration = 3,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 1,
	.can_enter = &can_enter_config3,
	.on_enter = &on_enter_config3,
	.on_exit = &on_exit_config3,
	.on_zero_request = 0,
	.on_in_request = 0,
	.on_out_request = 0,
};

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

static const usb_ep0_configuration_callbacks_t CONFIG6_CBS = {
	.configuration = 6,
	.interfaces = 1,
	.out_endpoints = 0,
	.in_endpoints = 1,
	.can_enter = &can_enter_config6,
	.on_enter = &on_enter_config6,
	.on_exit = &on_exit_config6,
	.on_zero_request = 0,
	.on_in_request = 0,
	.on_out_request = 0,
};

static const usb_ep0_configuration_callbacks_t * const CONFIG_CBS[] = {
	&CONFIG1_CBS,
	&CONFIG2_CBS,
	&CONFIG3_CBS,
	&CONFIG4_CBS,
	&CONFIG5_CBS,
	&CONFIG6_CBS,
	0
};

static const usb_device_info_t DEVICE_INFO = {
	.rx_fifo_words = 128,
	.ep0_max_packet = 8,
};

extern unsigned char linker_data_vma_start;
extern unsigned char linker_data_vma_end;
extern const unsigned char linker_data_lma_start;
extern unsigned char linker_bss_vma_start;
extern unsigned char linker_bss_vma_end;

static void stm32_main(void) {
	// Check if we're supposed to go to the bootloader
	uint32_t rcc_csr_shadow = RCC_CSR; // Keep a copy of RCC_CSR
	RCC_CSR |= 1 << 24; // RMVF = 1; clear reset flags
	RCC_CSR &= ~(1 << 24); // RMVF = 0; stop clearing reset flags
	if ((rcc_csr_shadow & (1 << 28) /* SFTRST */) && bootload_flag == UINT64_C(0xDEADBEEFCAFEBABE)) {
		bootload_flag = 0;
		asm volatile(
			"mov sp, %[stack]\n\t"
			"mov pc, %[vector]"
			:
			: [stack] "r" (*(const volatile uint32_t *) 0x1FFF0000), [vector] "r" (*(const volatile uint32_t *) 0x1FFF0004));
	}
	bootload_flag = 0;

	// Copy initialized globals and statics from ROM to RAM
	memcpy(&linker_data_vma_start, &linker_data_lma_start, &linker_data_vma_end - &linker_data_vma_start);
	// Scrub the BSS section in RAM
	memset(&linker_bss_vma_start, 0, &linker_bss_vma_end - &linker_bss_vma_start);

	// Always 8-byte-align the stack pointer on entry to an interrupt handler (as ARM recommends)
	SCS_CCR |= 1 << 9; // STKALIGN = 1; guarantee 8-byte alignment

	// As we will be running at 144 MHz, switch to the lower-power voltage regulator mode (compatible only up to 144 MHz)
	PWR_CR &= ~(1 << 14); // VOS = 0; set regulator scale 2

	// Enable the HSE (8 MHz crystal) oscillator
	RCC_CR =
		(1 << 16) // HSEON = 1; enable HSE oscillator
		| (16 << 3) // HSITRIM = 16; trim HSI oscillator to midpoint
		| (1 << 0); // HSION = 1; enable HSI oscillator for now as we're still using it
	// Wait for the HSE oscillator to be ready
	while (!(RCC_CR & (1 << 17) /* HSERDY */));
	// Configure the PLL
	RCC_PLLCFGR =
		(RCC_PLLCFGR & 0xF0F00004) // Reserved bits
		| (6 << 24) // PLLQ = 6; divide 288 MHz VCO output by 6 to get 48 MHz USB, SDIO, and RNG clock
		| (1 << 22) // PLLSRC = 1; use HSE for PLL input
		| (0 << 16) // PLLP = 0; divide 288 MHz VCO output by 2 to get 144 MHz SYSCLK
		| (144 << 6) // PLLN = 144; multiply 2 MHz VCO input by 144 to get 288 MHz VCO output
		| (4 << 0); // PLLM = 4; divide 8 MHz HSE by 4 to get 2 MHz VCO input
	// Enable the PLL
	RCC_CR |= (1 << 24); // PLLON = 1; enable PLL
	// Wait for the PLL to lock
	while (!(RCC_CR & (1 << 25) /* PLLRDY */));
	// Set up bus frequencies
	RCC_CFGR =
		(RCC_CFGR & 0x00000300) // Reserved bits
		| (2 << 30) // MCO2 = 2; MCO2 pin outputs HSE
		| (0 << 27) // MCO2PRE = 0; divide 8 MHz HSE by 1 to get 8 MHz MCO2 (must be ≤ 100 MHz)
		| (0 << 24) // MCO1PRE = 0; divide 8 MHz HSE by 1 to get 8 MHz MCO1 (must be ≤ 100 MHz)
		| (0 << 23) // I2SSRC = 0; I2S module gets clock from PLLI2X
		| (2 << 21) // MCO1 = 2; MCO1 pin outputs HSE
		| (8 << 16) // RTCPRE = 8; divide 8 MHz HSE by 8 to get 1 MHz RTC clock (must be 1 MHz)
		| (4 << 13) // PPRE2 = 4; divide 144 MHz AHB clock by 2 to get 72 MHz APB2 clock (must be ≤ 84 MHz)
		| (5 << 10) // PPRE1 = 5; divide 144 MHz AHB clock by 4 to get 36 MHz APB1 clock (must be ≤ 42 MHz)
		| (0 << 4) // HPRE = 0; divide 144 MHz SYSCLK by 1 to get 144 MHz AHB clock (must be ≤ 168 MHz)
		| (0 << 0); // SW = 0; use HSI for SYSCLK for now, until everything else is ready
	// Wait 16 AHB cycles for the new prescalers to settle
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
	// Set Flash access latency to 5 wait states
	FLASH_ACR =
		(FLASH_ACR & 0xFFFFE0F8) // Reserved bits
		| (4 << 0); // LATENCY = 4; four wait states (acceptable for 120 ≤ HCLK ≤ 150)
	// Flash access latency change may not be immediately effective; wait until it's locked in
	while ((FLASH_ACR & 7) != 4);
	// Actually initiate the clock switch
	RCC_CFGR = (RCC_CFGR & ~(3 << 0)) | (2 << 0); // SW = 2; use PLL for SYSCLK
	// Wait for the clock switch to complete
	while ((RCC_CFGR & (3 << 2)) != (2 << 2) /* SWS */);
	// Turn off the HSI now that it's no longer needed
	RCC_CR = (RCC_CR & ~(1 << 0)) | (0 << 0); // HSION = 0; disable HSI

	// Flush any data in the CPU caches (which are not presently enabled)
	FLASH_ACR |=
		(1 << 12) // DCRST = 1; reset data cache
		| (1 << 11); // ICRST = 1; reset instruction cache
	FLASH_ACR &=
		~(1 << 12) // DCRST = 0; stop resetting data cache
		& ~(1 << 11); // ICRST = 0; stop resetting instruction cache

	// Turn on the caches. There is an errata that says prefetching doesn't work on some silicon, but it seems harmless to enable the flag even so
	FLASH_ACR |=
		(1 << 10) // DCEN = 1; enable data cache
		| (1 << 9) // ICEN = 1; enable instruction cache
		| (1 << 8); // PRFTEN = 1; enable prefetching

	// Set SYSTICK to divide by 144 so it overflows every microsecond
	SCS_STRVR = 144 - 1;
	// Set SYSTICK to run with the core AHB clock
	SCS_STCSR =
		(SCS_STCSR & 0xFFFEFFF8) // Reserved bits
		| (1 << 2) // CLKSOURCE = 1; use core clock
		| (0 << 1) // TICKINT = 0; do not generate an interrupt on expiry
		| (1 << 0); // ENABLE = 1; counter is running
	// Reset the counter
	SCS_STCVR = 0;

	// Initialize subsystems
	buzzer_init();

	// Set up pins
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

	// Wait a bit
	sleep_1ms(100);

	// Turn off LEDs
	GPIOB_ODR &= ~(7 << 12);

	// Initialize USB
	usb_ep0_set_global_callbacks(&DEVICE_CBS);
	usb_ep0_set_configuration_callbacks(CONFIG_CBS);
	usb_attach(&DEVICE_INFO);
	NVIC_ISER(67 / 32) = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt

	// Handle activity
	for (;;) {
	}
}

