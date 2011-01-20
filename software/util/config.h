#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include "util/noncopyable.h"
#include <cstddef>
#include <glibmm.h>
#include <map>
#include <stdint.h>
#include <vector>

namespace xmlpp {
	class Element;
}

/**
 * Provides access to the configuration file.
 */
class Config : public NonCopyable {
	public:
		/**
		 * Loads the configuration file.
		 */
		Config();

		/**
		 * Saves the configuration file.
		 */
		void save() const;

		/**
		 * Gets the outbound radio channel.
		 *
		 * \return the outbound radio channel.
		 */
		unsigned int out_channel() const {
			return out_channel_;
		}

		/**
		 * Gets the inbound radio channel.
		 *
		 * \return the inbound radio channel.
		 */
		unsigned int in_channel() const {
			return in_channel_;
		}

		/**
		 * Sets the radio channels.
		 *
		 * \param[in] out the new outbound channel, which must be between \c 0x0B and \c 0x1A.
		 *
		 * \param[in] in the new inbound channel, which must be between \c 0x0B and \c 0x1A.
		 */
		void channels(unsigned int out, unsigned int in);

		/**
		 * The collection of tunable booleans, by name.
		 */
		std::map<Glib::ustring, bool> bool_params;

		/**
		 * The collection of tunable integers, by name.
		 */
		std::map<Glib::ustring, int> int_params;

		/**
		 * The collection of tunable doubles, by name.
		 */
		std::map<Glib::ustring, double> double_params;

	private:
		unsigned int out_channel_, in_channel_;
};

#endif

