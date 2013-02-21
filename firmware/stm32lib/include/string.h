#ifndef STM32LIB_STRING_H
#define STM32LIB_STRING_H

/**
 * \file
 *
 * \brief Provides a subset of the functionality found in the ISO C \c string.h header.
 */

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif

