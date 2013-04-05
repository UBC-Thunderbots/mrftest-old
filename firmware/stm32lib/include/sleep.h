#ifndef STM32LIB_SLEEP_H
#define STM32LIB_SLEEP_H

/**
 * \file
 *
 * \brief Provides the ability to busy-wait for periods of time.
 *
 * The functions in this module assume that the system tick timer has been initialized to one overflow per microsecond.
 */

/**
 * \cond INTERNAL_SYMBOLS
 */
void sleep_systick_overflows(unsigned long ticks);
/**
 * \endcond
 */

/**
 * \brief Sleeps for some number of milliseconds.
 *
 * \param x the number of milliseconds to sleep
 */
#define sleep_ms(x) sleep_us((x) * 1000UL)

/**
 * \brief Sleeps for some number of microseconds.
 *
 * \param x the number of microseconds to sleep
 */
#define sleep_us(x) sleep_systick_overflows((x))

#endif
