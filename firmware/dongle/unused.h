#ifndef UNUSED_H
#define UNUSED_H

/**
 * \file
 *
 * \brief Provides a macro for eliminating unused parameter warnings.
 */

/**
 * \brief Marks a function parameter as unused.
 *
 * This macro should be wrapped around the parameter name in the function definition’s parameter list.
 *
 * \param x the name of the parameter
 */
#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

#endif

