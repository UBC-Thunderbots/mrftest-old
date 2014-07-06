/**
 * \ingroup REG
 * \defgroup REGIWDG Independent watchdog
 * @{
 */
#ifndef STM32LIB_REGISTERS_IWDG_H
#define STM32LIB_REGISTERS_IWDG_H

#include <stdint.h>

#define IWDG_BASE 0x40003000U

#define IWDG_KR (*(volatile uint32_t *) (IWDG_BASE + 0x00U))
#define IWDG_PR (*(volatile uint32_t *) (IWDG_BASE + 0x04U))
#define IWDG_RLR (*(volatile uint32_t *) (IWDG_BASE + 0x08U))

typedef struct {
	unsigned PVU : 1;
	unsigned RVU : 1;
	unsigned : 30;
} IWDG_SR_t;
#define IWDG_SR (*(volatile IWDG_SR_t *) (IWDG_BASE + 0x0CU))

#endif

/**
 * @}
 */

