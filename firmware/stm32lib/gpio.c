#include <gpio.h>
#include <assert.h>
#include <rcc.h>

void gpio_init(const gpio_init_pin_t specs[4U][16U]) {
	rcc_enable(AHB1, GPIOA);
	rcc_enable(AHB1, GPIOB);
	rcc_enable(AHB1, GPIOC);
	rcc_enable(AHB1, GPIOD);
	rcc_reset(AHB1, GPIOA);
	rcc_reset(AHB1, GPIOB);
	rcc_reset(AHB1, GPIOC);
	rcc_reset(AHB1, GPIOD);
	for (unsigned int port = 0U; port < 4U; ++port) {
		uint32_t moder = 0U, otyper = 0U, ospeedr = 0U, pupdr = 0U, odr = 0U, afr[2U] = { 0U, 0U }, lock = 0U;
		for (unsigned int pin = 0U; pin < 16U; ++pin) {
			moder |= specs[port][pin].mode << (pin * 2U);
			otyper |= specs[port][pin].otype << pin;
			ospeedr |= specs[port][pin].ospeed << (pin * 2U);
			pupdr |= specs[port][pin].pupd << (pin * 2U);
			odr |= specs[port][pin].od << pin;
			afr[pin / 8U] |= specs[port][pin].af << ((pin % 8U) * 4U);
			if (!specs[port][pin].unlock) {
				lock |= 1U << pin;
			}
		}

		GPIO[port].ODR = odr;
		GPIO[port].OSPEEDR = ospeedr;
		GPIO[port].PUPDR = pupdr;
		GPIO[port].AFRL = afr[0U];
		GPIO[port].AFRH = afr[1U];
		GPIO[port].OTYPER = otyper;
		GPIO[port].MODER = moder;
		if (lock) {
			GPIO[port].LCKR = lock;
			GPIO[port].LCKR = lock | 0x10000U;
			GPIO[port].LCKR = lock;
			GPIO[port].LCKR = lock | 0x10000U;
			(void) GPIO[port].LCKR;
			assert(GPIO[port].LCKR & 0x10000U);
		}
	}
}

