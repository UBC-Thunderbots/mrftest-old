#ifndef UTIL_IHEX_H
#define UTIL_IHEX_H

#include <vector>
#include <glibmm.h>

/**
 * An Intel HEX file. A HEX file can contain memory for many different addresses
 * with many different layouts. To be generic, this class allows the caller to
 * define which ranges of addresses are valid before loading the file. The
 * caller does this by calling the add_section(unsigned int, unsigned int)
 * function. The load(const Glib::ustring &) function will refuse to load a HEX
 * file that contains data outside the predefined sections.
 */
class IntelHex {
	public:
		/**
		 * Adds a section descriptor.
		 *
		 * \param[in] start the first address in the section.
		 *
		 * \param[in] length the length, in bytes, of the section.
		 */
		void add_section(unsigned int start, unsigned int length);

		/**
		 * Loads data from a file.
		 *
		 * \param[in] filename the file to load data from.
		 */
		void load(const Glib::ustring &filename);

		/**
		 * Returns the data, keyed by section.
		 *
		 * \return an array of section data blocks.
		 */
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

		std::vector<Section> sections;
		std::vector<std::vector<unsigned char> > the_data;
};

#endif

