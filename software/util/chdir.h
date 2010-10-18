#ifndef UTIL_CHDIR_H
#define UTIL_CHDIR_H

#include "util/fd.h"
#include "util/noncopyable.h"

/**
 * Allows a temporary change of directory for a bounded scope.
 */
class ScopedCHDir : public NonCopyable {
	public:
		/**
		 * Changes to an alternate directory.
		 *
		 * \param[in] dir the directory to change to.
		 */
		ScopedCHDir(const char *dir);

		/**
		 * Returns to the original directory.
		 */
		~ScopedCHDir();

	private:
		FileDescriptor::Ptr old_dir;
};

#endif

