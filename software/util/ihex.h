#ifndef UTIL_IHEX_H
#define UTIL_IHEX_H

#include <vector>
#include <glibmm.h>

//
// An Intel HEX file.
//
class IntelHex {
	public:
		//
		// Adds a section descriptor.
		//
		void add_section(unsigned int start, unsigned int length);

		//
		// Loads data from a file.
		//
		void load(const Glib::ustring &filename);

		//
		// Returns the data, keyed by section.
		//
		const std::vector<std::vector<unsigned char> > &data() const {
			return the_data;
		}

	private:
		class Section {
			public:
				Section(unsigned int start, unsigned int length) : start_(start), length_(length) {
				}

				unsigned int start() const {
					return start_;
				}

				unsigned int length() const {
					return length_;
				}

			private:
				unsigned int start_, length_;
		};

		std::vector<Section> the_sections;
		std::vector<std::vector<unsigned char> > the_data;
};

#endif

