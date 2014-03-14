#ifndef STM32LIB_INIT_H
#define STM32LIB_INIT_h

/**
 * \file
 *
 * \brief Provides useful functionality for bringing up the STM32F4.
 */

#include <exception.h>
#include <stdbool.h>

typedef struct {
	/**
	 * \brief \c true if a crystal is attached to the HSE pins, or \c false if a canned oscillator is attached.
	 */
	bool hse_crystal : 1;

	/**
	 * \brief \c true if FreeRTOS will be used, or \c false if not.
	 *
	 * If FreeRTOS is used, it is given control over the MPU and Systick modules; if not, they are configured appropriately by this module.
	 */
	bool freertos : 1;
} init_specs_flags_t;

/**
 * \brief Provides details on what configuration the application wants.
 */
typedef struct {
	/**
	 * \brief Various flags controlling options.
	 */
	init_specs_flags_t flags;

	/**
	 * \brief The frequency of the HSE crystal or oscillator, in megahertz.
	 *
	 * This must be an even number between 2 and 50.
	 */
	unsigned int hse_frequency;

	/**
	 * \brief The frequency to run the PLL at, in megahertz.
	 *
	 * This must be a multiple of 48 between 192 and 432.
	 */
	unsigned int pll_frequency;

	/**
	 * \brief The frequency to run the system clock at, in megahertz.
	 *
	 * This must be either ½, ¼, ⅙, or ⅛ of \ref pll_frequency, and no more than 168.
	 */
	unsigned int sys_frequency;

	/**
	 * \brief The frequency to run the CPU core and AHB at, in megahertz.
	 *
	 * This must be 1, ½, ¼, ⅛, …, 1/512 of \ref sys_frequency.
	 */
	unsigned int cpu_frequency;

	/**
	 * \brief The frequency to run APB1 at, in megahertz.
	 *
	 * This must be 1, ½, ¼, ⅛, or 1/16 of \ref cpu_frequency, and no more than 42.
	 */
	unsigned int apb1_frequency;

	/**
	 * \brief The frequency to run APB2 at, in megahertz.
	 *
	 * This must be 1, ½, ¼, ⅛, or 1/16 of \ref cpu_frequency, and no more than 84.
	 */
	unsigned int apb2_frequency;

	/**
	 * \brief The core dump writer to be invoked when a fatal exception occurs.
	 *
	 * This may be null to not write core dumps.
	 */
	const exception_core_writer_t *exception_core_writer;

	/**
	 * \brief The application callbacks to invoke when a fatal exception occurs.
	 */
	exception_app_cbs_t exception_app_cbs;
} init_specs_t;

/**
 * \brief Initializes the chip.
 *
 * The following initializations are done:
 * \li The bootloader is entered, if requested.
 * \li The system stacks are initialized properly.
 * \li The data section is filled from ROM.
 * \li The BSS section is wiped.
 * \li Interrupt handling is configured.
 * \li The memory protection unit is initialized.
 * \li The HSE oscillator is enabled.
 * \li The PLL is enabled and configured.
 * \li Flash access latency is set.
 * \li The system clock is switched to the PLL.
 * \li CPU caches are cleared and enabled.
 * \li The systick timer is configured to overflow once per microsecond.
 *
 * A call to this function should be the first statement in the main function.
 */
void init_chip(const init_specs_t *specs);

/**
 * \brief Marks a request to enter the bootloader and reboots the chip.
 *
 * The subsequent call to \ref init_chip will actually enter the bootloader.
 */
void init_bootload(void);

#endif

