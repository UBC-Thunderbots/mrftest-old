#ifndef UTIL_FD_H
#define UTIL_FD_H

#include "util/byref.h"
#include <sys/types.h>

/**
 * A file descriptor that is safely closed on destruction.
 */
class FileDescriptor : public ByRef {
	public:
		/**
		 * A pointer to a FileDescriptor.
		 */
		typedef RefPtr<FileDescriptor> Ptr;

		/**
		 * Constructs a new FileDescriptor with a descriptor.
		 *
		 * \param[in] fd the existing file descriptor, of which ownership is taken.
		 *
		 * \return a new FileDescriptor owning \p fd.
		 */
		static Ptr create_from_fd(int fd);

		/**
		 * Constructs a new FileDescriptor by calling \c open(2).
		 *
		 * \param[in] file the name of the file to open or create.
		 *
		 * \param[in] flags the file flags to use as per \c open(2).
		 *
		 * \param[in] mode the permissions to create a new file with, if \c O_CREAT is included in \p flags.
		 *
		 * \return the new FileDescriptor.
		 */
		static Ptr create_open(const char *file, int flags, mode_t mode);

		/**
		 * Constructs a new FileDescriptor by calling \c socket(2).
		 *
		 * \param[in] pf the protocol family to create a socket in.
		 *
		 * \param[in] type the type of socket to create.
		 *
		 * \param[in] proto the specific protocol to create a socket for, or 0 to use the default protocol for a given \c pf and \c type.
		 *
		 * \return the new FileDescriptor.
		 */
		static Ptr create_socket(int pf, int type, int proto);

		/**
		 * Constructs a FileDescriptor that refers to a unique file that has not been opened by any other process,
		 * and that does not have any name on disk.
		 *
		 * \param[in] pattern the pattern for the filename, which must be suitable for \c mkstemp().
		 *
		 * \return the new descriptor.
		 */
		static Ptr create_temp(const char *pattern);

		/**
		 * Destroys a FileDescriptor.
		 */
		~FileDescriptor();

		/**
		 * Closes the descriptor.
		 */
		void close();

		/**
		 * Gets the actual file descriptor.
		 *
		 * \return the descriptor.
		 */
		int fd() const;

		/**
		 * Sets whether the descriptor is blocking.
		 *
		 * \param[in] block \c true to set the descriptor to blocking mode, or \c false to set the descriptor to non-blocking mode.
		 */
		void set_blocking(bool block) const;

	private:
		int fd_;

		FileDescriptor(int fd);
		FileDescriptor(const char *file, int flags, mode_t mode);
		FileDescriptor(int pf, int type, int proto);
		FileDescriptor(const char *pattern);
};

#endif

