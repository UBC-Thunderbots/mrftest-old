#ifndef RCC_H
#define RCC_H

/**
 * \file
 *
 * \brief Provides utilities for doing various operations on the reset and clock control registers
 */

#include "stdint.h"

// This function is an implementation detail and should not be called directly
static inline void rcc_enable_(uint32_t mask, volatile uint32_t *rstr, volatile uint32_t *enr) __attribute__((unused));
static inline void rcc_enable_(uint32_t mask, volatile uint32_t *rstr, volatile uint32_t *enr) {
	*rstr |= mask;
	asm volatile("dsb");
	asm volatile("nop");
	*enr |= mask;
	asm volatile("dsb");
	asm volatile("nop");
	*rstr &= ~mask;
	asm volatile("dsb");
	asm volatile("nop");
}

// This function is an implementation detail and should not be called directly
static inline void rcc_disable_(uint32_t mask, volatile uint32_t *enr) __attribute__((unused));
static inline void rcc_disable_(uint32_t mask, volatile uint32_t *enr) {
	*enr &= ~mask;
}

/**
 * \brief Resets and enables a module
 *
 * \param[in] bus the bus the module is attached to, one of AHB1, AHB2, AHB3, APB1, or APB2 (based on the name of the RCC registers controlling it)
 *
 * \param[in] bit the bit number of the module to enable
 */
#define rcc_enable(bus, bit) rcc_enable_(1 << (bit), &(RCC_ ## bus ## RSTR), &(RCC_ ## bus ## ENR))

/**
 * \brief Resets and enables multiple modules on the same bus
 *
 * \param[in] bus the bus the module is attached to, one of AHB1, AHB2, AHB3, APB1, or APB2 (based on the name of the RCC registers controlling it)
 *
 * \param[in] mask the bit mask of the modules to enable
 */
#define rcc_enable_multi(bus, mask) rcc_enable_((mask), &(RCC_ ## bus ## RSTR), &(RCC_ ## bus ## ENR))

/**
 * \brief Disables a module
 *
 * \param[in] bus the bus the module is attached to, one of AHB1, AHB2, AHB3, APB1, or APB2 (based on the name of the RCC registers controlling it)
 *
 * \param[in] bit the bit number of the module to disable
 */
#define rcc_disable(bus, bit) rcc_disable_(1 << (bit), &(RCC_ ## bus ## ENR))

/**
 * \brief Disables multiple modules on the same bus
 *
 * \param[in] bus the bus the module is attached to, one of AHB1, AHB2, AHB3, APB1, or APB2 (based on the name of the RCC registers controlling it)
 *
 * \param[in] mask the bit mask of the modules to disable
 */
#define rcc_disable_multi(bus, mask) rcc_disable_((mask), &(RCC_ ## bus ## ENR))

#endif

