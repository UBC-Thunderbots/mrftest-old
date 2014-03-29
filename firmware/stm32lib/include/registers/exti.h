/**
 * \ingroup REG
 * \defgroup REGEXTI External interrupt/event controller
 * @{
 */
#ifndef STM32LIB_REGISTERS_EXTI_H
#define STM32LIB_REGISTERS_EXTI_H

#include <stdint.h>

#define EXTI_BASE 0x40013C00

#define EXTI_IMR (*(volatile uint32_t *) (EXTI_BASE + 0x00))
#define EXTI_ERM (*(volatile uint32_t *) (EXTI_BASE + 0x04))
#define EXTI_RTSR (*(volatile uint32_t *) (EXTI_BASE + 0x08))
#define EXTI_FTSR (*(volatile uint32_t *) (EXTI_BASE + 0x0C))
#define EXTI_SWIER (*(volatile uint32_t *) (EXTI_BASE + 0x10))
#define EXTI_PR (*(volatile uint32_t *) (EXTI_BASE + 0x14))

#endif

/**
 * @}
 */

