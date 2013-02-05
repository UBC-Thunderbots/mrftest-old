#ifndef ASSERT_H
#define ASSERT_H

/**
 * \brief Checks a condition and calls \ref abort if false.
 *
 * \param cond the condition to check, which is not evaluated if the NDEBUG macro is defined
 */
#ifdef NDEBUG
#define assert(cond)
#else
#include "stdlib.h"
#define assert(cond) do { if (!(cond)) { abort(); } } while(0)
#endif

#endif

