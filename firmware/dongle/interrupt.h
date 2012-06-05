#ifndef INTERRUPT_H
#define INTERRUPT_H

/**
 * \file
 *
 * \brief Contains the machinery necessary to dispatch interrupts that can go to one of multiple destinations
 */

/**
 * \brief The handler for EXTI12 interrupts
 */
extern void (*interrupt_exti12_handler)(void);

#endif

