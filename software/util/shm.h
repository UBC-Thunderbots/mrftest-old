#ifndef UTIL_SHM_H
#define UTIL_SHM_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <stdint.h>

/**
 * A block of shared memory backed by a file.
 */
class RawShmBlock : public NonCopyable {
	public:
		/**
		 * Creates a new shared memory block in a new file.
		 *
		 * \param[in] sz the size of the block, in bytes.
		 */
		RawShmBlock(std::size_t sz);

		/**
		 * Opens an existing shared memory block backed by an open file.
		 *
		 * \param[in] fd the descriptor of the open file.
		 *
		 * \param[in] sz the size of the block, in bytes.
		 */
		RawShmBlock(FileDescriptor::Ptr fd, std::size_t sz);

		/**
		 * Destroys a shared memory block.
		 */
		~RawShmBlock();

		/**
		 * Gets the size of the shared memory block.
		 *
		 * \return the size of the shared memory block.
		 */
		std::size_t size() const {
			return size_;
		}

		/**
		 * Gets a pointer to the first byte of the block.
		 *
		 * \return a pointer to the first byte of the block.
		 */
		operator void *() {
			return data_;
		}

		/**
		 * Gets a pointer to the first byte of the block.
		 *
		 * \returns a pointer to the first byte of the block.
		 */
		operator const void *() const {
			return data_;
		}

		/**
		 * Gets a pointer to the first byte of the block.
		 *
		 * \return a pointer to the first byte of the block.
		 */
		void *get() {
			return data_;
		}

		/**
		 * Gets a pointer to the first byte of the block.
		 *
		 * \return a pointer to the first byte of the block.
		 */
		const void *get() const {
			return data_;
		}

		/**
		 * Gets the file descriptor holding the shared memory block.
		 *
		 * \return the file descriptor holding the shared memory block.
		 */
		int fd() const {
			return fd_->fd();
		}

	private:
		const FileDescriptor::Ptr fd_;
		std::size_t size_;
		uint8_t *data_;
};

/**
 * A shared memory block containing an object of a particular type.
 *
 * \tparam T the type of object held in the block.
 */
template<typename T> class ShmBlock : public NonCopyable {
	public:
		/**
		 * Creates a new shared memory block in a new file.
		 */
		ShmBlock() : raw(sizeof(T)) {
		}

		/**
		 * Opens an existing shared memory block backed by an open file.
		 *
		 * \param[in] fd the descriptor of the open file.
		 */
		ShmBlock(FileDescriptor::Ptr fd) : raw(fd, sizeof(T)) {
		}

		/**
		 * Gets a pointer to the object stored in the shared memory block.
		 *
		 * \return a pointer to the underlying object.
		 */
		operator T *() {
			return static_cast<T *>(raw.get());
		}

		/**
		 * Gets a pointer to the object stored in the shared memory block.
		 *
		 * \return a pointer to the underlying object.
		 */
		operator const T *() const {
			return static_cast<const T *>(raw.get());
		}

		/**
		 * Gets a pointer to the object stored in the shared memory block.
		 *
		 * \return a pointer to the underlying object.
		 */
		T *operator->() {
			return static_cast<T *>(raw.get());
		}

		/**
		 * Gets a pointer to the object stored in the shared memory block.
		 *
		 * \return a pointer to the underlying object.
		 */
		const T *operator->() const {
			return static_cast<const T *>(raw.get());
		}

		/**
		 * Gets the file descriptor holding the shared memory block.
		 *
		 * \return the file descriptor holding the shared memory block.
		 */
		int fd() const {
			return raw.fd();
		}

	private:
		RawShmBlock raw;
};

#endif

