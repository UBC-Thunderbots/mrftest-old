#include "registers.h"
#include "sleep.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

static void stm32_main(void);
static void nmi_vector(void) __attribute__((interrupt));
static void hard_fault_vector(void) __attribute__((interrupt));
static void memory_manage_vector(void) __attribute__((interrupt));
static void bus_fault_vector(void) __attribute__((interrupt));
static void usage_fault_vector(void) __attribute__((interrupt));
static void service_call_vector(void) __attribute__((interrupt));
static void pending_service_vector(void) __attribute__((interrupt));
static void system_tick_vector(void) __attribute__((interrupt));

static char stack[65536] __attribute__((section(".stack")));

typedef void (*fptr)(void);
static const fptr vectors[16 + 82] __attribute__((used, section(".vectors"))) = {
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

static uint64_t bootload_flag;

extern const unsigned int foo;
extern unsigned int bar, baz;

static const uint8_t DEVICE_DESCRIPTOR[18] = {
	18, // bLength
	1, // bDescriptorType
	0, // bcdUSB LSB
	2, // bcdUSB MSB
	0xFF, // bDeviceClass
	0, // bDeviceSubClass
	0, // bDeviceProtocol
	64, // bMaxPacketSize0
	0x23, // idVendor LSB
	0x01, // idVendor MSB
	0x67, // idProduct LSB
	0x45, // idProduct MSB
	0, // bcdDevice LSB
	0, // bcdDevice MSB
	0, // iManufacturer
	0, // iProduct
	0, // iSerialNumber
	1, // bNumConfigurations
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

	// Enable the HSE (8 MHz crystal) oscillator
	RCC_CR =
		(1 << 16) // HSEON = 1; enable HSE oscillator
		| (16 << 3) // HSITRIM = 16; trim HSI oscillator to midpoint
		| (1 << 0); // HSION = 1; enable HSI oscillator for now as we're still using it
	// Wait for the HSE oscillator to be ready
	while (!(RCC_CR & (1 << 17) /* HSERDY */));
	// Configure the PLL. The VCO's input will be 2 MHz and its output will be 336 MHz
	RCC_PLLCFGR =
		(RCC_PLLCFGR & 0xF0F00004) // Reserved bits
		| (7 << 24) // PLLQ = 7; divide 336 MHz VCO output by 7 to get 48 MHz for USB, SDIO, and RNG
		| (1 << 22) // PLLSRC = 1; use HSE for PLL input
		| (0 << 16) // PLLP = 0; divide 336 MHz VCO output by 2 to get 168 MHz for SYSCLK
		| (168 << 6) // PLLN = 168; multiply 2 MHz VCO input by 168 to get 336 MHz output
		| (4 << 0); // PLLM = 4; divide 8 MHz HSE by 4 to get 2 MHz VCO input
	// Enable the PLL
	RCC_CR |= (1 << 24); // PLLON = 1; enable PLL
	// Wait for the PLL to lock
	while (!(RCC_CR & (1 << 25) /* PLLRDY */));
	// Set up bus frequencies
	RCC_CFGR =
		(RCC_CFGR & 0x00000300) // Reserved bits
		| (0 << 30) // MCO2 = 0; MCO2 pin outputs SYSCLK
		| (4 << 27) // MCO2PRE = 4; divide 168 MHz SYSCLK by 2 to get 84 MHz MCO2 (must be ≤ 100 MHz)
		| (4 << 24) // MCO1PRE = 0; divide 8 MHz HSE by 1 to get 8 MHz MCO1 (must be ≤ 100 MHz)
		| (0 << 23) // I2SSRC = 0; I2S module gets clock from PLLI2X
		| (2 << 21) // MCO1 = 2; MCO1 pin outputs HSE
		| (8 << 16) // RTCPRE = 8; divide 8 MHz HSE by 8 to get 1 MHz RTC clock (must be 1 MHz)
		| (4 << 13) // PPRE2 = 4; divide 168 MHz AHB clock by 2 to get 84 MHz APB2 clock (must be ≤ 84 MHz)
		| (5 << 10) // PPRE1 = 5; divide 168 MHz AHB clock by 4 to get 42 MHz APB1 clock (must be ≤ 42 MHz)
		| (0 << 4) // HPRE = 0; divide 168 MHz SYSCLK by 1 to get 168 MHz AHB clock (must be ≤ 168 MHz)
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
		| (5 << 0); // LATENCY = 5; five wait states
	// Flash access latency change may not be immediately effective; wait until it's locked in
	while ((FLASH_ACR & 7) != 5);
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

	// Set SYSTICK to divide by 16,800 so it overflows every 100 µs
	SCS_STRVR = 16800 - 1;
	// Set SYSTICK to run with the core AHB clock
	SCS_STCSR =
		(SCS_STCSR & 0xFFFEFFF8) // Reserved bits
		| (1 << 2) // CLKSOURCE = 1; use core clock
		| (0 << 1) // TICKINT = 0; do not generate an interrupt on expiry
		| (1 << 0); // ENABLE = 1; counter is running
	// Reset the counter
	SCS_STCVR = 0;

	// Enable the clocks to peripherals
	RCC_AHB1ENR |=
		(1 << 20) // CCMDATARAMEN = 1; enable clock to core-coupled memory
		| (1 << 3) // GPIODEN = 1; enable clock to GPIOD
		| (1 << 2) // GPIOCEN = 1; enable clock to GPIOC
		| (1 << 1) // GPIOBEN = 1; enable clock to GPIOB
		| (1 << 0); // GPIOAEN = 1; enable clock to GPIOA
	RCC_AHB2ENR |= (1 << 7); // OTGFSEN = 1; enable clock to USB FS
	RCC_APB2ENR |= (1 << 12); // SPI1EN = 1; enable clock to SPI 1
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	// Set up pins
	// PA15 = MRF /CS, start deasserted
	// PA14/PA13 = alternate function SWD
	// PA12/PA11 = alternate function OTG FS
	// PA10/PA9/PA8/PA7/PA6 = N/C
	// PA5/PA4 = shorted to VDD
	// PA3 = shorted to VSS
	// PA2 = buzzer, start off
	// PA1/PA0 = shorted to VDD
	GPIOA_ODR = 0b1000000000110011;
	GPIOA_OSPEEDR = 0b01000000000000000000000000000000;
	GPIOA_PUPDR = 0b00100100000000000000000000000000;
	GPIOA_AFRH = 0b00000000000010101010000000000000;
	GPIOA_AFRL = 0b00000000000000000000000000000000;
	GPIOA_MODER = 0b01101010100101010101010101010101;
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
	sleep_millis(100);

	// Release MRF24J40 from reset
	GPIOB_ODR |= 1 << 7;
	sleep_millis(100);

	// Read register 0x12 (ACKTMOUT); should have initial value 0b00111001 = 0x39
	{
		// Configure and enable the SPI module
		SPI1_CR1 = (0 << 15) // BIDIMODE = 0; use 2-line unidirectional mode
			| (0 << 13) // CRCEN = 0; do not do hardware CRC calculation
			| (0 << 12) // CRCNEXT = 0; do not calculate CRC at this time
			| (0 << 11) // DFF = 0; send and receive 8-bit frames
			| (0 << 10) // RXONLY = 0; enable both transmission and reception
			| (1 << 9) // SSM = 1; in-transceiver chip select logic is controlled by software, not a pin
			| (1 << 8) // SSI = 1; transceiver should assume /CS pin is deasserted
			| (0 << 7) // LSBFIRST = 0; MSb is sent first
			| (1 << 6) // SPE = 1; module enabled
			| (3 << 3) // BR = 0b011; baud rate = fPCLK / 16 = 84 MHz / 16 = 5.25 MHz
			| (1 << 2) // MSTR = 1; master mode
			| (0 << 1) // CPOL = 0; clock idles low
			| (0 << 0); // CPHA = 0; first edge (rising) is capture edge
		sleep_millis(1);
		// Assert chip select
		GPIOA_ODR &= ~(1 << 15);
		sleep_millis(1);
		// Issue the command
		SPI1_DR = 0x12 << 1;
		while (!(SPI1_SR & (1 << 1) /* TXE */));
		SPI1_DR = 0;
		while (!(SPI1_SR & (1 << 0) /* RXNE */));
		(void) SPI1_DR;
		// Read the result;
		while (!(SPI1_SR & (1 << 0) /* RXNE */));
		unsigned char buffer = SPI1_DR;
		while (!(SPI1_SR & (1 << 1) /* TXE */));
		while (SPI1_SR & (1 << 7) /* BSY */);
		// Deassert chip select
		GPIOA_ODR |= 1 << 15;
		if (SPI1_SR & ((1 << 8) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3))) {
			GPIOB_ODR &= ~(1 << 12);
			for (;;);
		}
		// Deactivate the module
		SPI1_CR1 &= ~(1 << 6); // SPE = 0; module disabled
		// Check the result
		if (buffer != 0x39) {
			GPIOB_ODR &= ~(1 << 13);
			for (;;);
		}
	}

	// Blink some LEDs and beep
	unsigned int i = 0;
	for (;;) {
		i = (i + 1) & 7;
		GPIOB_ODR = i << 12;
		if (i == 0) {
			for (unsigned int i = 0; i < 1000; ++i) {
				GPIOA_ODR = 1 << 2;
				sleep_systick_overflows(2);
				GPIOA_ODR = 0;
				sleep_systick_overflows(3);
				GPIOA_ODR = 1 << 2;
				sleep_systick_overflows(3);
				GPIOA_ODR = 0;
				sleep_systick_overflows(2);
			}
		} else {
			sleep_millis(1000);
		}
	}
}

