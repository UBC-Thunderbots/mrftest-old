#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <core_progmem.h>
#include <exception.h>
#include <gpio.h>
#include <init.h>
#include <inttypes.h>
#include <rcc.h>
#include <stddef.h>
#include <registers/id.h>
#include <registers/iwdg.h>
#include <registers/otg_fs.h>
#include <registers/systick.h>

#include "pins.h"
#include "main.h"

static void stm32_main(void) __attribute__((noreturn));

static unsigned long mstack[1024U] __attribute__((section(".mstack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16U] __attribute__((used, section(".exception_vectors"))) = {
	[0U] = (fptr) (mstack + sizeof(mstack) / sizeof(*mstack)),
	[1U] = &stm32_main,
	[3U] = &exception_hard_fault_isr,
	[4U] = &exception_memory_manage_fault_isr,
	[5U] = &exception_bus_fault_isr,
	[6U] = &exception_usage_fault_isr,
};


static const init_specs_t INIT_SPECS = {
	.flags = {
		.hse_crystal = false,
		.freertos = false,
		.io_compensation_cell = false,	//I don't think any signals will be running at >50MHz?
	},

	.hse_frequency = 8,
	.pll_frequency = 336,
	.sys_frequency = 168,
	.cpu_frequency = 168,
	.apb1_frequency = 42,
	.apb2_frequency = 84,
	.exception_core_writer = NULL,	//Don't have any
	.exception_app_cbs = {
		.early = NULL,
		.late = NULL,
	},
};


	
#define BLINKED_LED GPIOD,13U


static void stm32_main(void)
{
	volatile uint32_t i;
	init_chip(&INIT_SPECS);

	gpio_init(PINS_INIT,sizeof(PINS_INIT)/sizeof(*PINS_INIT));

	while (1)
	{
		gpio_set(BLINKED_LED);	

		for (i = 0; i < 1000000; i++ ){}

		gpio_reset(BLINKED_LED);

		for (i = 0; i < 1000000; i++) {}
	}


}


