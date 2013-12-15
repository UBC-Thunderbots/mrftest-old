#ifndef STM32LIB_EXCEPTION_H
#define STM32LIB_EXCEPTION_H

/**
 * \file
 *
 * \brief Provides a framework for handling CPU exceptions.
 */

#include <stdbool.h>
#include <stddef.h>

/**
 * \brief The functionality that must be provided by a core dump writer.
 */
typedef struct {
	/**
	 * \brief Starts writing a core dump.
	 */
	void (*start)(void);

	/**
	 * \brief Writes a block of data to the core dump.
	 *
	 * \param[in] data the data to write
	 *
	 * \param[in] length the number of bytes to write, which is guaranteed to be a multiple of four
	 */
	void (*write)(const void *, size_t);

	/**
	 * \brief Finishes writing a core dump.
	 *
	 * \return \c true if the dump was written successfully, or \c false if an error occurred saving the core dump
	 */
	bool (*end)(void);
} exception_core_writer_t;

/**
 * \brief The functionality that an application can provide to be invoked during a fatal error.
 */
typedef struct {
	/**
	 * \brief Invoked almost immediately when an exception is taken.
	 *
	 * This callback should safe any dangerous hardware, perhaps display a preliminary indication, and return.
	 *
	 * This may be null to not execute an early callback.
	 */
	void (*early)(void);

	/**
	 * \brief Invoked after exception handling is complete.
	 *
	 * This callback should display an indication to the user indefinitely.
	 * It should generally not return.
	 *
	 * This may be null to just lock up forever with no further activity.
	 *
	 * \param[in] core_written \c true if a core dump was written, or \c false if not
	 */
	void (*late)(bool);
} exception_app_cbs_t;

/**
 * \brief Initializes the exception handling subsystem.
 *
 * This function sets up interrupt priorities and enables the various internal CPU exceptions.
 * It also chooses how core dumps will be saved.
 *
 * \param[in] writer the writer to use, which may be null to omit core dumping altogether
 *
 * \param[in] the application callbacks, which may be null to omit application callbacks altogether
 */
void exception_init(const exception_core_writer_t *core_writer, const exception_app_cbs_t *app_cbs);

/**
 * \brief An interrupt handler that handles hard faults.
 *
 * This function must be inserted as element 3 in the application’s CPU exception vector table.
 */
void exception_hard_fault_vector(void) __attribute__((naked, noreturn));

/**
 * \brief An interrupt handler that handles memory manage faults.
 *
 * This function must be inserted as element 4 in the application’s CPU exception vector table.
 */
void exception_memory_manage_fault_vector(void) __attribute__((naked, noreturn));

/**
 * \brief An interrupt handler that handles bus faults.
 *
 * This function must be inserted as element 5 in the application’s CPU exception vector table.
 */
void exception_bus_fault_vector(void) __attribute__((naked, noreturn));

/**
 * \brief An interrupt handler that handles usage faults.
 *
 * This function must be inserted as element 6 in the application’s CPU exception vector table.
 */
void exception_usage_fault_vector(void) __attribute__((naked, noreturn));

#endif

