#ifndef UTIL_SHM_H
#define UTIL_SHM_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <stdint.h>

//
// A block of shared memory backed by a file.
//
class RawShmBlock : public NonCopyable {
	public:
		//
		// Creates a new shared memory block in a new file.
		//
		RawShmBlock(std::size_t);

		//
		// Opens an existing shared memory block backed by an open file.
		//
		RawShmBlock(FileDescriptor, std::size_t);

		//
		// Destroys a shared memory block.
		//
		~RawShmBlock();

		//
		// Returns the size of the shared memory block.
		//
		std::size_t size() const {
			return size_;
		}

		//
		// Returns a pointer to the first byte of the block.
		//
		operator void *() {
			return data_;
		}

		//
		// Returns a pointer to the first byte of the block.
		//
		operator const void *() const {
			return data_;
		}

		//
		// Returns a pointer to the first byte of the block.
		//
		void *get() {
			return data_;
		}

		//
		// Returns a pointer to the first byte of the block.
		//
		const void *get() const {
			return data_;
		}

		//
		// Returns the file descriptor holding the shared memory block.
		//
		int fd() const {
			return fd_;
		}

	private:
		FileDescriptor fd_;
		std::size_t size_;
		uint8_t *data_;
};

//
// A shared memory block containing an object of a particular type.
//
template<typename T>
class ShmBlock : public NonCopyable {
	public:
		//
		// Creates a new shared memory block in a new file.
		//
		ShmBlock() : raw(sizeof(T)) {
		}

		//
		// Opens an existing shared memory block backed by an open file.
		//
		ShmBlock(FileDescriptor fd) : raw(fd, sizeof(T)) {
		}

		//
		// Allows access to the underlying object.
		//
		operator T *() {
			return static_cast<T *>(raw.get());
		}

		// 
		// Allows access to the underlying object.
		//
		operator const T *() const {
			return static_cast<const T *>(raw.get());
		}

		//
		// Allows access to the underlying object.
		//
		T *operator->() {
			return static_cast<T *>(raw.get());
		}

		// 
		// Allows access to the underlying object.
		//
		const T *operator->() const {
			return static_cast<const T *>(raw.get());
		}

		//
		// Returns the file descriptor holding the shared memory block.
		//
		int fd() const {
			return raw.fd();
		}

	private:
		RawShmBlock raw;
};

#endif

