#include <init.h>
#include <assert.h>
#include <inttypes.h>
#include <rcc.h>
#include <registers/flash.h>
#include <registers/mpu.h>
#include <registers/power.h>
#include <registers/scb.h>
#include <registers/systick.h>
#include <registers/syscfg.h>
#include <stdlib.h>
#include <string.h>

volatile uint64_t bootload_flag;
#define BOOTLOAD_FLAG_VALUE UINT64_C(0xFE228106195AD2B0)

static char mstack[32768] __attribute__((section(".mstack")));

extern unsigned char linker_data_vma;
extern const unsigned char linker_data_lma;
extern unsigned char linker_data_size;
extern unsigned char linker_bss_vma;
extern unsigned char linker_bss_size;

static unsigned int compute_ahb_prescale(unsigned int sys, unsigned int cpu) {
	assert(!(sys % cpu));
	switch (sys / cpu) {
		case 1: return 0;
		case 2: return 8;
		case 4: return 9;
		case 8: return 10;
		case 16: return 11;
		case 64: return 12;
		case 128: return 13;
		case 256: return 14;
		case 512: return 15;
		default: abort();
	}
}

static unsigned int compute_apb_prescale(unsigned int cpu, unsigned int apb) {
	assert(!(cpu % apb));
	switch (cpu / apb) {
		case 1: return 0;
		case 2: return 4;
		case 4: return 5;
		case 8: return 6;
		case 16: return 7;
		default: abort();
	}
}

static unsigned int compute_flash_wait_states(unsigned int cpu) {
	return (cpu - 1) / 30;
}

void init_chip(const init_specs_t *specs) {
	// Check if we’re supposed to go to the bootloader.
	RCC_CSR_t rcc_csr_shadow = RCC_CSR; // Keep a copy of RCC_CSR
	RCC_CSR.RMVF = 1; // Clear reset flags
	RCC_CSR.RMVF = 0; // Stop clearing reset flags
	if (rcc_csr_shadow.SFTRSTF && bootload_flag == BOOTLOAD_FLAG_VALUE) {
		bootload_flag = 0;
		rcc_enable(APB2, SYSCFG);
		rcc_reset(APB2, SYSCFG);
		SYSCFG_MEMRMP.MEM_MODE = 1;
		asm volatile(
			"msr control, %[control]\n\t"
			"isb\n\t"
			"msr msp, %[stack]\n\t"
			"mov pc, %[vector]"
			:
			: [control] "r" (0), [stack] "r" (*(const volatile uint32_t *) 0x1FFF0000), [vector] "r" (*(const volatile uint32_t *) 0x1FFF0004));
		__builtin_unreachable();
	}
	bootload_flag = 0;

	// Copy the main stack pointer (MSP) to the process stack pointer (PSP) and start using the process stack.
	asm volatile(
			"mrs r0, msp\n\t"
			"msr psp, r0\n\t"
			"mov r0, #2\n\t"
			"msr control, r0\n\t"
			"isb"
			:
			:
			: "cc", "r0");

	// Point main stack pointer (MSP) at the main stack.
	asm volatile(
			"msr msp, %[new_msp]\n\t"
			"isb"
			:
			: [new_msp] "r" (mstack + sizeof(mstack) / sizeof(*mstack))
			: "cc");

	// Copy initialized globals and statics from ROM to RAM.
	memcpy(&linker_data_vma, &linker_data_lma, (size_t) &linker_data_size /* Yes, there should be an & here! */);
	// Scrub the BSS section in RAM.
	memset(&linker_bss_vma, 0, (size_t) &linker_bss_size /* Yes, there should be an & here! */);

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
	exception_init(specs->exception_core_writer, &specs->exception_app_cbs);

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

	// Enable the HSE oscillator.
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
			.PLLSRC = 1, // Use HSE for PLL input
		};

		// Divide HSE frequency to get 2 MHz input to PLL.
		assert(!(specs->hse_frequency % 2));
		assert(2 <= specs->hse_frequency && specs->hse_frequency <= 50);
		tmp.PLLM = specs->hse_frequency / 2;

		// Multiply 2 MHz input to get PLL output frequency.
		assert(!(specs->pll_frequency % 2));
		assert(192 <= specs->pll_frequency && specs->pll_frequency <= 432);
		tmp.PLLN = specs->pll_frequency / 2;

		// Divide PLL output to get system freqency.
		assert(specs->sys_frequency <= 168);
		assert(!(specs->pll_frequency % specs->sys_frequency));
		switch (specs->pll_frequency / specs->sys_frequency) {
			case 2: tmp.PLLP = 0; break;
			case 4: tmp.PLLP = 1; break;
			case 6: tmp.PLLP = 2; break;
			case 8: tmp.PLLP = 3; break;
			default: abort(); break;
		}

		// Divide PLL output to get 48 MHz USB/SDIO/RNG clock.
		assert(!(specs->pll_frequency % 48));
		tmp.PLLQ = specs->pll_frequency / 48;
		
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
			.MCO2PRE = 0, // Divide HSE by 1 to get MCO2 (must be ≤ 100 MHz)
			.MCO1PRE = 0, // Divide HSE by 1 to get MCO1 (must be ≤ 100 MHz)
			.I2SSRC = 0, // I²S module gets clock from PLLI2X
			.MCO1 = 2, // MCO1 pin outputs HSE
			.RTCPRE = 0, // RTC clock disabled
			.SW = 0, // Use HSI for SYSCLK for now, until everything else is ready
		};

		// Divide system clock frequency to get CPU frequency.
		assert(specs->cpu_frequency <= 168);
		tmp.HPRE = compute_ahb_prescale(specs->sys_frequency, specs->cpu_frequency);

		// Divide CPU frequency to get APB1 and APB2 frequency.
		assert(specs->apb1_frequency <= 42);
		tmp.PPRE1 = compute_apb_prescale(specs->cpu_frequency, specs->apb1_frequency);
		assert(specs->apb2_frequency <= 84);
		tmp.PPRE2 = compute_apb_prescale(specs->cpu_frequency, specs->apb2_frequency);

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
	// Set Flash access latency to necessary number of wait states.
	unsigned int flash_wait_states = compute_flash_wait_states(specs->cpu_frequency);
	{
		FLASH_ACR_t tmp = {
			.DCRST = 0, // Do not clear data cache at this time.
			.ICRST = 0, // Do not clear instruction cache at this time.
			.DCEN = 0, // Do not enable data cache at this time.
			.ICEN = 0, // Do not enable instruction cache at this time.
			.PRFTEN = 0, // Do not enable prefetcher at this time.
		};
		tmp.LATENCY = flash_wait_states;
		FLASH_ACR = tmp;
	}
	// Flash access latency change may not be immediately effective; wait until it’s locked in.
	while (FLASH_ACR.LATENCY != flash_wait_states);
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

	// Set SYSTICK to divide by cpu_frequency so it overflows every microsecond.
	SYST_RVR.RELOAD = specs->cpu_frequency - 1;
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

	// If we will be running at at most 144 MHz, switch to the lower-power voltage regulator mode.
	if (specs->cpu_frequency <= 144) {
		rcc_enable(APB1, PWR);
		rcc_reset(APB1, PWR);
		PWR_CR.VOS = 2; // Set regulator scale 2
		rcc_disable(APB1, PWR);
	}
}

void init_bootload(void) {
	// Mark that we should go to the bootloader on next reboot.
	bootload_flag = BOOTLOAD_FLAG_VALUE;

	// Disable all interrupts.
	asm volatile("cpsid i");
	asm volatile("isb");

	// Request the reboot.
	{
		AIRCR_t tmp = AIRCR;
		tmp.VECTKEY = 0x05FA;
		tmp.SYSRESETREQ = 1;
		AIRCR = tmp;
	}
	asm volatile("dsb");

	// Wait forever until the reboot happens.
	for (;;) {
		asm volatile("wfi");
	}
}

