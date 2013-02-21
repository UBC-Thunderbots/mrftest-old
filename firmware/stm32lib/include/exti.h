#ifndef EXTI_H
#define EXTI_H

/**
 * \file
 *
 * \brief Handles the external interrupt pin configuration.
 */

/**
 * \brief The type of an interrupt handler for an EXTI interrupt.
 */
typedef void (*exti_handler_t)(void);

/**
 * \brief Connects a specific GPIO pin to an internal EXTI signal.
 *
 * In the STM32F4 series, there are 15 independent EXTI signals, numbered from 0 to 15.
 * Each EXTI signal can be wired to the GPIO pin with the same number on any of the nine ports, PA through PI—EXTI3 can be connected to PA3, PB3, PC3, and so on, for example.
 *
 * \param line the EXTI number to configure, from 0 to 15
 *
 * \param port the port to connect the EXTI to, from 0 (PA) to 8 (PI)
 */
void exti_map(unsigned int line, unsigned int port);

/**
 * \brief Sets the handler function that handles an EXTI interrupt.
 *
 * Multiple EXTI interrupts share the same physical hardware interrupt vector, so this machinery is needed to separate those interrupts out.
 *
 * \param line the EXTI number to configure, from 0 to 15
 *
 * \param handler the handler that should be invoked when the specified EXTI signals an interrupt condition
 */
void exti_set_handler(unsigned int line, exti_handler_t handler);

#endif

