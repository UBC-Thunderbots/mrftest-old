#ifndef UTIL_FD_H
#define UTIL_FD_H

#include "util/noncopyable.h"

//
// A file descriptor that is safely closed on destruction.
//
class file_descriptor : public virtual noncopyable {
	public:
		//
		// Constructs a new file_descriptor.
		//
		file_descriptor(int fd);

		//
		// Destroys a file_descriptor.
		//
		virtual ~file_descriptor();

		//
		// Returns the descriptor.
		//
		operator int() const {
			return fd;
		}

	private:
		int fd;
};

#endif

