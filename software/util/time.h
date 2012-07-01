#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include "util/string.h"
#include <ctime>
#include <glibmm/ustring.h>

/**
 * Gets the current time into a timespec.
 *
 * \param[out] result the location at which to store the current time.
 *
 * \param[in] clk which clock to query.
 */
void timespec_now(timespec *result, clockid_t clk = CLOCK_MONOTONIC);

namespace {
	/**
	 * Gets the current time into a timespec.
	 *
	 * \param[out] result the location at which to store the current time.
	 *
	 * \param[in] clk which clock to query.
	 */
	void timespec_now(timespec &result, clockid_t clk = CLOCK_MONOTONIC) {
		timespec_now(&result, clk);
	}

	/**
	 * Adds a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \param[out] result a location at which to store the value of \p ts1 + \p ts2.
	 */
	void timespec_add(const timespec &ts1, const timespec &ts2, timespec &result) {
		result.tv_sec = ts1.tv_sec + ts2.tv_sec;
		result.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
		if (result.tv_nsec >= 1000000000L) {
			++result.tv_sec;
			result.tv_nsec -= 1000000000L;
		}
	}

	/**
	 * Adds a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \return \p ts1 + \p ts2.
	 */
	timespec timespec_add(const timespec &ts1, const timespec &ts2) {
		timespec result;
		timespec_add(ts1, ts2, result);
		return result;
	}

	/**
	 * Subtracts a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \param[out] result a location at which to store the value of \p ts1 − \p ts2.
	 */
	void timespec_sub(const timespec &ts1, const timespec &ts2, timespec &result) {
		if (ts1.tv_nsec >= ts2.tv_nsec) {
			result.tv_sec = ts1.tv_sec - ts2.tv_sec;
			result.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
		} else {
			result.tv_sec = ts1.tv_sec - ts2.tv_sec - 1;
			result.tv_nsec = ts1.tv_nsec + 1000000000L - ts2.tv_nsec;
		}
	}

	/**
	 * Subtracts a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \return \p ts1 − \p ts2.
	 */
	timespec timespec_sub(const timespec &ts1, const timespec &ts2) {
		timespec result;
		timespec_sub(ts1, ts2, result);
		return result;
	}

	/**
	 * Compares a pair of timespecs.
	 *
	 * \param[in] ts1 the first timespec.
	 *
	 * \param[in] ts2 the second timespec.
	 *
	 * \return a positive value if \p ts1 > \p ts2, a negative value if \p ts1 < \p ts2, or zero if \p ts1 = \p ts2.
	 */
	int timespec_cmp(const timespec &ts1, const timespec &ts2) {
		if (ts1.tv_sec != ts2.tv_sec) {
			return ts1.tv_sec > ts2.tv_sec ? 1 : -1;
		} else if (ts1.tv_nsec != ts2.tv_nsec) {
			return ts1.tv_nsec > ts2.tv_nsec ? 1 : -1;
		} else {
			return 0;
		}
	}

	/**
	 * Converts a timespec to a double-precision count of seconds.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the number of seconds represented by \p ts.
	 */
	double timespec_to_double(const timespec &ts) {
		return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1000000000.0;
	}

	/**
	 * Converts a double-precision count of seconds to a timespec.
	 *
	 * \param[in] seconds the second count to convert.
	 *
	 * \return the timespec.
	 */
	timespec double_to_timespec(double seconds) {
		timespec ts;
		ts.tv_sec = static_cast<time_t>(seconds);
		ts.tv_nsec = static_cast<long>((seconds - static_cast<double>(ts.tv_sec)) * 1000000000.0);
		if (ts.tv_nsec < 0L) {
			ts.tv_nsec = 0L;
		}
		if (ts.tv_nsec >= 1000000000L) {
			ts.tv_nsec = 999999999L;
		}
		return ts;
	}

	/**
	 * Converts a timespec to an integer count of milliseconds.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the number of milliseconds represented by \p ts.
	 */
	unsigned int timespec_to_millis(const timespec &ts) {
		return static_cast<unsigned int>(ts.tv_sec * 1000U + ts.tv_nsec / 1000000U);
	}

	/**
	 * \brief Converts a timespec to an integer count of nanoseconds.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the number of nanoseconds represented by \p ts.
	 */
	unsigned int timespec_to_nanos(const timespec &ts) {
		return static_cast<unsigned int>(ts.tv_sec * 1000000000U + ts.tv_nsec);
	}

	/**
	 * \brief Converts a timespec to a human-readable string representation.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the string representation.
	 */
	Glib::ustring timespec_to_string(const timespec &ts) {
		if (ts.tv_sec >= 0) {
			return Glib::ustring::compose("%1.%2", ts.tv_sec, todecu(static_cast<unsigned long>(ts.tv_nsec), 9));
		} else if (ts.tv_nsec == 0) {
			return Glib::ustring::compose("-%1.000000000", -ts.tv_sec);
		} else {
			return Glib::ustring::compose("-%1.%2", -(ts.tv_sec + 1), todecu(1000000000UL - static_cast<unsigned long>(ts.tv_nsec), 9));
		}
	}

	/**
	 * \brief Converts a timespec to a machine-readable string representation suitable for e.g. writing to CSV files.
	 *
	 * \param[in] ts the timespec to convert.
	 *
	 * \return the string representation.
	 */
	Glib::ustring timespec_to_string_machine(const timespec &ts) {
		if (ts.tv_sec >= 0) {
			return Glib::ustring::compose("%1.%2", todecu(static_cast<uintmax_t>(ts.tv_sec), 0), todecu(static_cast<unsigned long>(ts.tv_nsec), 9));
		} else if (ts.tv_nsec == 0) {
			return Glib::ustring::compose("-%1.000000000", todecu(static_cast<uintmax_t>(-ts.tv_sec), 0));
		} else {
			return Glib::ustring::compose("-%1.%2", todecu(static_cast<uintmax_t>(-(ts.tv_sec + 1)), 0), todecu(1000000000UL - static_cast<unsigned long>(ts.tv_nsec), 9));
		}
	}
}

#endif

