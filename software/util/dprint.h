#ifndef UTIL_DPRINT_H
#define UTIL_DPRINT_H

#include <glibmm/ustring.h>
#include <sigc++/signal.h>
#include "proto/log_record.pb.h"

/**
 * A signal emitted every time a message is logged.
 */
extern sigc::signal<void, Log::DebugMessageLevel, const Glib::ustring &>
    signal_message_logged;

void log_impl(
    const char *file, unsigned int line, const char *function,
    const Glib::ustring &msg, Log::DebugMessageLevel level);

/**
 * Outputs a message at the debug log level.
 * The message will not be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_DEBUG(msg)                                                         \
    log_impl(__FILE__, __LINE__, __func__, msg, Log::DEBUG_MESSAGE_LEVEL_DEBUG)

/**
 * Outputs a message at the informational log level.
 * The message will be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_INFO(msg)                                                          \
    log_impl(__FILE__, __LINE__, __func__, msg, Log::DEBUG_MESSAGE_LEVEL_INFO)
#define LOGF_INFO(msg, ...) LOG_INFO(Glib::ustring::compose(msg, __VA_ARGS__))

/**
 * Outputs a message at the warning log level.
 * The message will be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_WARN(msg)                                                          \
    log_impl(__FILE__, __LINE__, __func__, msg, Log::DEBUG_MESSAGE_LEVEL_WARN)

/**
 * Outputs a message at the error log level.
 * The message will be shown on the terminal.
 *
 * \param msg the message.
 */
#define LOG_ERROR(msg)                                                         \
    log_impl(__FILE__, __LINE__, __func__, msg, Log::DEBUG_MESSAGE_LEVEL_ERROR)

#endif
