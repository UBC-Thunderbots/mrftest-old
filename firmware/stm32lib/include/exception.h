#ifndef EXCEPTION_H
#define EXCEPTION_H

/**
 * \file
 *
 * \brief Provides a framework for handling CPU exceptions.
 */

/**
 * \brief Initializes the exception handling subsystem.
 *
 * This function sets up interrupt priorities and enables the various internal CPU exceptions.
 */
void exception_init(void);

/**
 * \brief An interrupt handler that handles hard faults.
 *
 * This function must be inserted as element 3 in the application’s CPU exception vector table.
 */
void exception_hard_fault_vector(void) __attribute__((naked));

/**
 * \brief An interrupt handler that handles memory manage faults.
 *
 * This function must be inserted as element 4 in the application’s CPU exception vector table.
 */
void exception_memory_manage_fault_vector(void) __attribute__((naked));

/**
 * \brief An interrupt handler that handles bus faults.
 *
 * This function must be inserted as element 5 in the application’s CPU exception vector table.
 */
void exception_bus_fault_vector(void) __attribute__((naked));

/**
 * \brief An interrupt handler that handles usage faults.
 *
 * This function must be inserted as element 6 in the application’s CPU exception vector table.
 */
void exception_usage_fault_vector(void) __attribute__((naked));

#endif

