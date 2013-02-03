#ifndef MINMAX_H
#define MINMAX_H

/**
 * \file
 *
 * \brief Provides definitions of the \ref MIN and \ref MAX macros.
 */

/**
 * \brief Returns the smaller of two elements.
 *
 * \param[in] a the first element
 *
 * \param[in] b the second element
 *
 * \return the smaller element
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * \brief Returns the larger of two elements.
 *
 * \param[in] a the first element
 *
 * \param[in] b the second element
 *
 * \return the larger element
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif

