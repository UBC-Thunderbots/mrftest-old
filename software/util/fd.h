#ifndef UTIL_FD_H
#define UTIL_FD_H

#include <cassert>

/**
 * A file descriptor that is safely closed on destruction.
 * Ownership semantics are equivalent to std::auto_ptr.
 */
class FileDescriptor {
	private:
		/**
		 * This is an implementation detail used to help with copying.
		 */
#warning just make it use byref like everything else
		class FileDescriptorRef {
			public:
				FileDescriptorRef(int newfd) : fd(newfd) {
				}

				int fd;
		};

	public:
		/**
		 * Constructs a new FileDescriptor with a descriptor.
		 *
		 * \param[in] fd the existing file descriptor, of which ownership is
		 * taken.
		 *
		 * \return a new FileDescriptor owning \p fd.
		 */
		static FileDescriptor create(int fd) {
			FileDescriptor obj;
			obj = fd;
			return obj;
		}

		/**
		 * Constructs a new FileDescriptor with no descriptor.
		 */
		FileDescriptor() : fd(-1) {
		}

		/**
		 * Helps to copy a FileDescriptor object.
		 *
		 * \param[in,out] ref the object to take a descriptor from.
		 */
		FileDescriptor(FileDescriptorRef ref) : fd(ref.fd) {
			assert(fd >= 0);
		}

		/**
		 * Constructs a new FileDescriptor by trying to open a file.
		 *
		 * \param[in] file the name of the file to open.
		 *
		 * \param[in] flags the file flags to use as per \c open(2).
		 */
		FileDescriptor(const char *file, int flags);

		/**
		 * Constructs a new FileDescriptor for a socket.
		 *
		 * \param[in] pf the protocol family to create a socket in.
		 *
		 * \param[in] type the type of socket to create.
		 *
		 * \param[in] proto the specific protocol to create a socket for, or 0
		 * to use the default protocol for a given \c pf and \c type.
		 */
		FileDescriptor(int pf, int type, int proto);

		/**
		 * Copies a FileDescriptor, transferring ownership.
		 *
		 * \param[in,out] copyref the object to take a descriptor from.
		 */
		FileDescriptor(FileDescriptor &copyref) {
			assert(copyref.fd != -1);
			fd = copyref.fd;
			copyref.fd = -1;
		}

		/**
		 * Destroys a FileDescriptor.
		 */
		~FileDescriptor() {
			close();
		}

		/**
		 * Helps to copy a FileDescriptor object.
		 */
		operator FileDescriptorRef() {
			int value = fd;
			fd = -1;
			return FileDescriptorRef(value);
		}

		/**
		 * Assigns a new value to a FileDescriptor.
		 *
		 * \param[in,out] assgref the object to take a descriptor from.
		 */
		FileDescriptor &operator=(FileDescriptor &assgref) {
			assert(assgref.fd != -1);
			if (assgref.fd != fd) {
				close();
				fd = assgref.fd;
				assgref.fd = -1;
			}
			return *this;
		}

		/**
		 * Assigns a new value to a FileDescriptor.
		 *
		 * \param[in,out] assgref the object to take a descriptor from.
		 */
		FileDescriptor &operator=(FileDescriptorRef assgref) {
			assert(assgref.fd != -1);
			close();
			fd = assgref.fd;
			return *this;
		}

		/**
		 * Closes the descriptor.
		 */
		void close();

		/**
		 * \return the descriptor.
		 */
		operator int() const {
			assert(fd != -1);
			return fd;
		}

		/**
		 * Sets whether the descriptor is blocking.
		 *
		 * \param[in] block \c true to set the descriptor to blocking mode, or
		 * \c false to set the descriptor to non-blocking mode.
		 */
		void set_blocking(bool block) const;

	private:
		int fd;
};

#endif

