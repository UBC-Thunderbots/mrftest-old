#ifndef DEFERRED_H
#define DEFERRED_H

/**
 * \file
 *
 * \brief Provides a framework for handling CPU exceptions and setting up deferred functions.
 *
 * \warning The services in this file are only for use by non-FreeRTOS-enabled applications!
 */

/**
 * \brief The type of a registerable handler structure for a deferred function call.
 *
 * Applications should not touch the contents of this structure.
 */
typedef struct deferred_fn {
	struct deferred_fn *next;
	void (*fn)(void *);
	void *cookie;
} deferred_fn_t;

/**
 * \brief A \ref deferred_fn_t should be assigned this initializer when defined.
 */
#define DEFERRED_FN_INIT { .next = 0, .fn = 0, .cookie = 0, }

/**
 * \brief Registers a function for deferred execution.
 *
 * The deferred functions will be executed in the order they were registered.
 * It is legal to call this registration function from within a deferred function (including reregistering the same structure).
 * It is also legal to pass the same structure a second time; this will change the function and cookie to be executed.
 * Reregistering an already-registered structure will not change its position in the queue.
 *
 * \param deferred the information structure identifying the deferred call
 *
 * \param fn the function to invoke (which may be null to cancel a registered deferred call)
 *
 * \param cookie an opaque value to pass to the function
 */
void deferred_fn_register(deferred_fn_t *deferred, void (*fn)(void *), void *cookie);

/**
 * \brief Handles pending system call CPU exceptions.
 *
 * This function must be inserted as element 14 in the application’s CPU exception vector table.
 */
void deferred_fn_pendsv_handler(void);

#endif

