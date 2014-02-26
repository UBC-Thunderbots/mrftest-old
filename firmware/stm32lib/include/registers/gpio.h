#ifndef STM32LIB_REGISTERS_GPIO_H
#define STM32LIB_REGISTERS_GPIO_H

/**
 * \file
 *
 * \brief Defines the general purpose I/O registers.
 */

#include <stdint.h>

#define GPIO_BASE 0x40020000

typedef struct {
	unsigned BS : 16;
	unsigned BR : 16;
} GPIO_BSRR_t;

typedef enum {
	GPIO_MODE_IN,
	GPIO_MODE_OUT,
	GPIO_MODE_AF,
	GPIO_MODE_AN,
	GPIO_MODE_MASK = 3,
} GPIO_MODE_t;

typedef enum {
	GPIO_OTYPE_PP,
	GPIO_OTYPE_OD,
	GPIO_OTYPE_MASK = 1,
} GPIO_OTYPE_t;

typedef enum {
	GPIO_OSPEED_2,
	GPIO_OSPEED_25,
	GPIO_OSPEED_50,
	GPIO_OSPEED_100,
	GPIO_OSPEED_MASK = 3,
} GPIO_OSPEED_t;

typedef enum {
	GPIO_PUPD_NONE,
	GPIO_PUPD_PU,
	GPIO_PUPD_PD,
	GPIO_PUPD_MASK = 3,
} GPIO_PUPD_t;

typedef struct {
	uint32_t MODER;
	uint32_t OTYPER;
	uint32_t OSPEEDR;
	uint32_t PUPDR;
	uint32_t IDR;
	uint32_t ODR;
	GPIO_BSRR_t BSRR;
	uint32_t LCKR;
	uint32_t AFRL;
	uint32_t AFRH;
	uint32_t pad[0x400U / 4U - 10U];
} GPIO_t;

typedef GPIO_t GPIOS_t[9];

#define GPIO (*(volatile GPIOS_t *) GPIO_BASE)
#define GPIOA (GPIO[0])
#define GPIOB (GPIO[1])
#define GPIOC (GPIO[2])
#define GPIOD (GPIO[3])
#define GPIOE (GPIO[4])
#define GPIOF (GPIO[5])
#define GPIOG (GPIO[6])
#define GPIOH (GPIO[7])
#define GPIOI (GPIO[8])

#endif

