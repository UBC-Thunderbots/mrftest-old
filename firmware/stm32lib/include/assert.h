#ifndef STM32LIB_ASSERT_H
#define STM32LIB_ASSERT_H

/**
 * \brief Checks a condition and calls the standard library’s \c abort if false.
 *
 * \param cond the condition to check, which is not evaluated if the NDEBUG macro is defined
 */
#ifdef NDEBUG
#define assert(cond)
#else
#include <abort.h>
#define assert(cond) do { \
	if (!(cond)) { \
		abort_cause.cause = ABORT_CAUSE_ASSERTION_FAILURE; \
		abort_cause.detail[0] = __LINE__; \
		abort_cause.detail[1] = ((const uint32_t *) __FILE__)[0]; \
		abort_cause.detail[2] = ((const uint32_t *) __FILE__)[1]; \
		abort(); \
	} \
} while(0)
#endif

#endif

