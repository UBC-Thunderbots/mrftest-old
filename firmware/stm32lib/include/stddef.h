#ifndef STM32LIB_STDDEF_H
#define STM32LIB_STDDEF_H

/**
 * \file
 *
 * \brief Provides the functionality found in the ISO C \c stddef.h header.
 */

#define NULL ((void *) 0)

#define offsetof(TYPE, ELT) __builtin_offsetof(TYPE, ELT)

typedef long ptrdiff_t;
typedef int wchar_t;
typedef unsigned long size_t;

#endif

