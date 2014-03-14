#ifndef STM32LIB_EXTI_H
#define STM32LIB_EXTI_H

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

/**
 * \brief Handles EXTI0 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 6.
 */
void exti0_isr(void);

/**
 * \brief Handles EXTI1 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 7.
 */
void exti1_isr(void);

/**
 * \brief Handles EXTI2 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 8.
 */
void exti2_isr(void);

/**
 * \brief Handles EXTI3 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 9.
 */
void exti3_isr(void);

/**
 * \brief Handles EXTI4 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 10.
 */
void exti4_isr(void);

/**
 * \brief Handles EXTI5 through EXTI9 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 23.
 */
void exti5_9_isr(void);

/**
 * \brief Handles EXTI10 through EXTI15 interrupts.
 *
 * This function should be registered in the application’s interrupt vector table at position 40.
 */
void exti10_15_isr(void);

#endif

