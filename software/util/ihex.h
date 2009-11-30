#ifndef UTIL_IHEX_H
#define UTIL_IHEX_H

#include <vector>
#include <glibmm.h>

//
// An Intel HEX file.
//
class intel_hex {
	public:
		//
		// Loads data from a file.
		//
		void load(const Glib::ustring &filename);

		//
		// Returns the data.
		//
		const std::vector<unsigned char> &data() const {
			return the_data;
		}

	private:
		std::vector<unsigned char> the_data;
};

#endif

