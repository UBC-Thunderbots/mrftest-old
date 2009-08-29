#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <cstddef>

//
// A circular buffer.
//
template<typename T, std::size_t N>
class circular_buffer {
	public:
		//
		// Constructs a new circular buffer.
		//
		circular_buffer();

		//
		// Gets the object at position "i" in the buffer, where 0 is the
		// most-recently added, 1 is the next-most-recently-added, and so on.
		//
		const T &operator[](const std::size_t i) const;

		//
		// Gets the object at position "i" in the buffer, where 0 is the
		// most-recently added, 1 is the next-most-recently-added, and so on.
		//
		T &operator[](const std::size_t i);

		//
		// Adds a new object to the buffer.
		//
		void add(const T &t);

		//
		// The size of the buffer.
		//
		const std::size_t size = N;

	private:
		T data[N];
		std::size_t ptr;
};

#endif

