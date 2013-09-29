#ifndef STM32LIB_ABORT_H
#define STM32LIB_ABORT_H

/**
 * \file
 *
 * \brief Provides a subset of the functionality found in the ISO C \c stdlib.h header.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * \brief The general classes of aborts.
 */
typedef enum {
	ABORT_CAUSE_UNKNOWN,
	ABORT_CAUSE_HARD_FAULT,
	ABORT_CAUSE_MEMORY_MANAGE_FAULT,
	ABORT_CAUSE_BUS_FAULT,
	ABORT_CAUSE_USAGE_FAULT,
	ABORT_CAUSE_ASSERTION_FAILURE,
} abort_cause_class_t;

/**
 * \brief Information about the cause of the abort.
 */
typedef struct {
	/**
	 * \brief The general class of abort.
	 */
	abort_cause_class_t cause;

	/**
	 * \brief Detail words whose meaning is specific to the abort class.
	 */
	uint32_t detail[3];
} abort_cause_t;

/**
 * \brief The cause of an abort.
 */
extern abort_cause_t abort_cause;

#endif

