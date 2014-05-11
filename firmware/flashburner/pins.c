#include "pins.h"

const gpio_init_pin_t PINS_INIT[4U][16U] = {
	{
		// PA0 = shorted to VDD, driven high
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PA1 = shorted to VDD, driven high
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PA2 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA3 = shorted to VSS, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA4 = internal Flash /CS, start deasserted (high)
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_25, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PA5 = shorted to VDD, driven high
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PA6 = N/C
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA7 = N/C
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA8 = N/C
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA9 = OTG FS VBUS, input
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA10 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA11 = alternate function OTG FS
		{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_25, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 10 },
		// PA12 = alternate function OTG FS
		{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_25, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 10 },
		// PA13 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PA14 = PROGRAM_B, input with no resistors until needed
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0, .unlock = true },
		// PA15 = external Flash /CS, input with no resistors until needed
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_25, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0, .unlock = true },
	},
	{
		// PB0 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB1 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB2 = BOOT1, hardwired low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB3 = external Flash SCK, input with no resistors until needed
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_50, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 5, .unlock = true },
		// PB4 = external Flash MISO, input with no resistors until needed
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 5, .unlock = true },
		// PB5 = external Flash MOSI, input with no resistors until needed
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_50, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 5, .unlock = true },
		// PB6 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB7 = alternate function USART 1 receive
		{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 7, .unlock = true },
		// PB8 = shorted to VSS, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB9 = shorted to VSS, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB10 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB11 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PB12 = LED 1, start high (on)
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PB13 = LED 2, start high (on)
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PB14 = LED 3, start high (on)
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
		// PB15 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
	},
	{
		// PC0 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC1 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC2 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC3 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC4 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC5 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC6 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC7 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC8 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC9 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC10 = alternate function internal Flash (SPI3) SCK
		{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_50, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 6 },
		// PC11 = alternate function internal Flash (SPI3) MISO
		{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_50, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 6 },
		// PC12 = alternate function internal Flash (SPI3) MOSI
		{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_50, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 6 },
		// PC13 = N/C, driven low
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PC14 = switch SW3 to ground (input with pull-up)
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_PU, .od = 0, .af = 0 },
		// PC15 = switch SW2 to ground (input with pull-up)
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_PU, .od = 0, .af = 0 },
	},
	{
		// PD0 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD1 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD2 = external power control, input with no resistors until needed
		{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0, .unlock = true },
		// PD3 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD4 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD5 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD6 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD7 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD8 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD9 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD10 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD11 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD12 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD13 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD14 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		// PD15 = unimplemented on package
		{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
	},
};

