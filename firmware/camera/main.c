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

static const init_specs_t INIT_SPECS = {
	.flags = {
		.hse_crystal = false,
		.freertos = false,
		.io_compensation_cell = false,	//I don't think any signals will be running at >50MHz?
	},

	.hse_frequency = 8,
	.pll_frequency = 336,
	.sys_frequency = 168,
	.apb1_frequency = 42,
	.apb2_frequency = 84,
	.exception_core_writer = NULL,	//Don't have any
	.exception_app_cbs = {
		.early = NULL,
		.late = NULL,
	},
};


	
#define BLINKED_LED GPIOB, 13U


static void stm32_main(void)
{
	init_chip(&INIT_SPECS);

	gpio_init(PINS_INIT,5U);

	while (1)
	{
		gpio_set(BLINKED_LED);
		int i;

		for (i = 0; i < 2000; i++ ){}

		gpio_reset(BLINKED_LED);

		for (i = 0; i < 2000; i++) {}
	}


}


