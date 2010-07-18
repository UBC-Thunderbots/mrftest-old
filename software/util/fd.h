#ifndef UTIL_FD_H
#define UTIL_FD_H

#include <cassert>

//
// A file descriptor that is safely closed on destruction.
// Ownership semantics are equivalent to std::auto_ptr.
//
class FileDescriptor {
	private:
		//
		// This is an implementation detail used to help with copying.
		//
#warning just make it use byref like everything else
		class FileDescriptorRef {
			public:
				FileDescriptorRef(int newfd) : fd(newfd) {
				}

				int fd;
		};

	public:
		//
		// Constructs a new FileDescriptor with a descriptor.
		//
		static FileDescriptor create(int fd) {
			FileDescriptor obj;
			obj = fd;
			return obj;
		}

		//
		// Constructs a new FileDescriptor with no descriptor.
		//
		FileDescriptor() : fd(-1) {
		}

		//
		// Helps to copy a FileDescriptor object.
		//
		FileDescriptor(FileDescriptorRef ref) : fd(ref.fd) {
			assert(fd >= 0);
		}

		//
		// Constructs a new FileDescriptor by trying to open a file.
		//
		FileDescriptor(const char *file, int flags);

		//
		// Constructs a new FileDescriptor for a socket.
		//
		FileDescriptor(int pf, int type, int proto);

		//
		// Copies a FileDescriptor, transferring ownership.
		//
		FileDescriptor(FileDescriptor &copyref) {
			assert(copyref.fd != -1);
			fd = copyref.fd;
			copyref.fd = -1;
		}

		//
		// Destroys a FileDescriptor.
		//
		~FileDescriptor() {
			close();
		}

		//
		// Helps to copy a FileDescriptor object.
		//
		operator FileDescriptorRef() {
			int value = fd;
			fd = -1;
			return FileDescriptorRef(value);
		}

		//
		// Assigns a new value to a FileDescriptor.
		//
		FileDescriptor &operator=(FileDescriptor &assgref) {
			assert(assgref.fd != -1);
			if (assgref.fd != fd) {
				close();
				fd = assgref.fd;
				assgref.fd = -1;
			}
			return *this;
		}

		//
		// Assigns a new value to a FileDescriptor.
		//
		FileDescriptor &operator=(FileDescriptorRef assgref) {
			assert(assgref.fd != -1);
			close();
			fd = assgref.fd;
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

