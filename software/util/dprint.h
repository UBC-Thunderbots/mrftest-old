#ifndef UTIL_DPRINT_H
#define UTIL_DPRINT_H

#include <glibmm.h>
#include <stdint.h>

/**
 * Converts an unsigned integer of any type to a fixed-width decimal string.
 *
 * \param[in] value the value to convert.
 *
 * \param[in] width the width, in characters, of the output to produce.
 *
 * \return the decimal string.
 */
Glib::ustring todec(uintmax_t value, unsigned int width);

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

/**
 * A signal emitted every time a message is logged.
 */
extern sigc::signal<void, unsigned int, const Glib::ustring &> signal_message_logged;

void log_impl(const char *file, unsigned int line, const char *function, const Glib::ustring &msg, unsigned int level);

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3

/**
 * Outputs a message at the debug log level.
 * The message will not be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_DEBUG(msg) log_impl(__FILE__, __LINE__, __func__, msg, LOG_LEVEL_DEBUG)

/**
 * Outputs a message at the informational log level.
 * The message will be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_INFO(msg) log_impl(__FILE__, __LINE__, __func__, msg, LOG_LEVEL_INFO)

/**
 * Outputs a message at the warning log level.
 * The message will be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_WARN(msg) log_impl(__FILE__, __LINE__, __func__, msg, LOG_LEVEL_WARN)

/**
 * Outputs a message at the error log level.
 * The message will be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_ERROR(msg) log_impl(__FILE__, __LINE__, __func__, msg, LOG_LEVEL_ERROR)

#endif

