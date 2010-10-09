#ifndef UTIL_MAPPED_FILE_H
#define UTIL_MAPPED_FILE_H

#include "util/noncopyable.h"
#include <string>

/**
 * A memory-mapped view of a file.
 */
class MappedFile : public NonCopyable {
	public:
		/**
		 * Maps in a file.
		 *
		 * \param[in] filename the file to map.
		 */
		MappedFile(const std::string &filename);

		/**
		 * Unmaps the file.
		 */
		~MappedFile();

		/**
		 * Returns the mapped data.
		 *
		 * \return the mapped data.
		 */
		const void *data() const {
			return data_;
		}

		/**
		 * Returns the size of the file.
		 *
		 * \return the size of the file.
		 */
		std::size_t size() const {
			return size_;
		}

	private:
		void *data_;
		std::size_t size_;
};

#endif

