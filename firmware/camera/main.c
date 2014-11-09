#include <stdint.h>
#include <stdio.h>

static const init_specs_t INIT_SPECS = {
	.flags = {
		.hse_crystal = false,
		.frertos = false,
		.io_compensation_cell = false,	//I don't think any signals will be running at >50MHz?
	};

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


		

static void stm32_main(void)
{
	init_chip(&INIT_SPECS);

	gpio_init(PINS_INIT);


}


