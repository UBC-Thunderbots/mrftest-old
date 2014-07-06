#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

/**
 * \ingroup MAIN
 *
 * \brief The sources that are checked for liveness by the supervisor task.
 */
typedef enum {
	MAIN_WDT_SOURCE_TICK, ///< The normal ticks.
	MAIN_WDT_SOURCE_HSTICK, ///< The fast ticks.
	MAIN_WDT_SOURCE_COUNT, ///< The number of liveness sources.
} main_wdt_source_t;

void main_kick_wdt(main_wdt_source_t source);
void main_shutdown(bool reboot);

#endif

