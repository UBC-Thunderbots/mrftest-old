#ifndef FORMAT_H
#define FORMAT_H

/**
 * \file
 *
 * \brief Provides functions for formatting numbers into strings.
 */

#include "stdint.h"

/**
 * \brief Formats a 4-bit nybble as a single hexadecimal digit.
 *
 * \param[in] dest the location at which to store the digit
 *
 * \param[in] val the nybble to convert
 */
void formathex4(char *dest, uint8_t val);

/**
 * \brief Formats an 8-bit byte as two hexadecimal digts.
 *
 * \param dest the location at which to store the digits
 *
 * \param val the byte to convert
 */
void formathex8(char *dest, uint8_t val);

/**
 * \brief Formats a 16-bit word as four hexadecimal digits.
 *
 * \param dest the location at which to store the digits
 *
 * \param val the word to convert
 */
void formathex16(char *dest, uint16_t val);

/**
 * \brief Formats a 32-bit word as eight hexadecimal digits.
 *
 * \param dest the location at which to store the digits
 *
 * \param val the word to convert
 */
void formathex32(char *dest, uint32_t val);

#endif

