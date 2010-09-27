#ifndef UTIL_DPRINT_H
#define UTIL_DPRINT_H

#include <stdint.h>
#include <glibmm.h>

/**
 * Converts an unsigned integer of any type to a hexadecimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the hex string.
 */
Glib::ustring tohex(uintmax_t value, unsigned int width);

void log_impl(const char *file, unsigned int line, const Glib::ustring &msg, unsigned int level);

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3

#define LOG_DEBUG(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_DEBUG)
#define LOG_INFO(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_INFO)
#define LOG_WARN(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_WARN)
#define LOG_ERROR(msg) log_impl(__FILE__, __LINE__, msg, LOG_LEVEL_ERROR)

#endif

