#ifndef UTIL_FD_H
#define UTIL_FD_H

#include <cassert>

//
// A file descriptor that is safely closed on destruction.
// Ownership semantics are equivalent to std::auto_ptr.
//
class file_descriptor {
	public:
		//
		// Constructs a new file_descriptor with no descriptor.
		//
		file_descriptor() : fd(-1) {
		}

		//
		// Constructs a new file_descriptor with a descriptor.
		//
		file_descriptor(int fd) : fd(fd) {
		}

		//
		// Constructs a new file_descriptor by trying to open a file.
		//
		file_descriptor(const char *file, int flags);

		//
		// Constructs a new file_descriptor for a socket.
		//
		file_descriptor(int pf, int type, int proto);

		//
		// Copies a file_descriptor, transferring ownership.
		//
		file_descriptor(file_descriptor &copyref) {
			assert(copyref.fd != -1);
			fd = copyref.fd;
			copyref.fd = -1;
		}

		//
		// Destroys a file_descriptor.
		//
		~file_descriptor() {
			close();
		}

		//
		// Assigns a new value to a file_descriptor.
		//
		file_descriptor &operator=(file_descriptor &assgref) {
			assert(assgref.fd != -1);
			close();
			fd = assgref.fd;
			assgref.fd = -1;
			return *this;
		}

		//
		// Assigns a new value to a file_descriptor.
		//
		file_descriptor &operator=(int newfd) {
			assert(newfd != -1);
			close();
			fd = newfd;
			return *this;
		}

		//
		// Closes the descriptor.
		//
		void close();

		//
		// Returns the descriptor.
		//
		operator int() const {
			assert(fd != -1);
			return fd;
		}

		//
		// Sets whether the descriptor is blocking.
		//
		void set_blocking(bool block) const;

	private:
		int fd;
};

#endif

