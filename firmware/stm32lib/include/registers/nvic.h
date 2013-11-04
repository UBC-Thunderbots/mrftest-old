#ifndef STM32LIB_REGISTERS_NVIC_H
#define STM32LIB_REGISTERS_NVIC_H

/**
 * \file
 *
 * \brief Defines the nested vectored interrupt controller registers.
 */

#include <stdint.h>

typedef uint32_t NVIC_ibits_t[8];
#define NVIC_ISER (*(volatile NVIC_ibits_t *) 0xE000E100)
#define NVIC_ICER (*(volatile NVIC_ibits_t *) 0xE000E180)
#define NVIC_ISPR (*(volatile NVIC_ibits_t *) 0xE000E200)
#define NVIC_ICPR (*(volatile NVIC_ibits_t *) 0xE000E280)
#define NVIC_IABR (*(volatile NVIC_ibits_t *) 0xE000E300)

typedef uint32_t NVIC_inybbles_t[60];
#define NVIC_IPR (*(volatile NVIC_inybbles_t *) 0xE000E400)

typedef struct {
	unsigned INTID : 9;
	unsigned : 23;
} STIR_t;
#define STIR (*(volatile STIR_t *) 0xE000EF00)

#endif

