#ifndef UTIL_BOX_ARRAY_H
#define UTIL_BOX_ARRAY_H

#include "util/box_ptr.h"
#include "util/exception.h"
#include "util/noncopyable.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>

namespace BoxArrayUtils {
	void *allocate_aligned_memory(std::size_t length);
}

/**
 * \brief An array of boxes that hold objects that can be referred to by BoxPtrs.
 *
 * \tparam T the type of object to hold in the boxes.
 *
 * \tparam N the number of elements in the array.
 */
template<typename T, std::size_t N> class BoxArray : public NonCopyable {
	public:
		/**
		 * \brief The number of elements in the array.
		 */
		static const std::size_t SIZE = N;

		/**
		 * \brief Constructs a new BoxArray with no objects in the boxes.
		 */
		BoxArray();

		/**
		 * \brief Destroys a BoxArray.
		 */
		~BoxArray();

		/**
		 * \brief Fetches a pointer to the object at a particular location.
		 *
		 * \param[in] i the index of the element to fetch.
		 *
		 * \return a pointer to the element.
		 */
		BoxPtr<T> operator[](std::size_t i) const;

		/**
		 * \brief Fills a box at a particular location with a newly-constructed element.
		 *
		 * If the box is already full, the element is destroyed then recreated.
		 *
		 * \tparam Args the types of the parameters to T's constructor.
		 *
		 * \param[in] i the index of the element to create.
		 *
		 * \param[in] args the parameters to pass to the object's constructor.
		 */
		template<typename ... Args> void create(std::size_t i, Args ... args);

		/**
		 * \brief Empties the box at a particular location.
		 *
		 * If the box is already empty, no action is taken.
		 *
		 * \param[in] i the index of the element to destroy.
		 */
		void destroy(std::size_t i);

	private:
		bool valid[N];
		T *buffer;
};



template<typename T, std::size_t N> BoxArray<T, N>::BoxArray() {
	std::fill(valid, valid + N, false);
	buffer = static_cast<T *>(BoxArrayUtils::allocate_aligned_memory(N * sizeof(T)));
	std::memset(static_cast<void *>(buffer), 0, N * sizeof(T));
}

template<typename T, std::size_t N> BoxArray<T, N>::~BoxArray() {
	for (std::size_t i = 0; i < N; ++i) {
		destroy(i);
	}
	std::free(buffer);
}

template<typename T, std::size_t N> BoxPtr<T> BoxArray<T, N>::operator[](std::size_t i) const {
	return BoxPtr<T>(&buffer[i], &valid[i]);
}

template<typename T, std::size_t N> template<typename ... Args> void BoxArray<T, N>::create(std::size_t i, Args ... args) {
	destroy(i);
	new(&buffer[i]) T(args...);
	valid[i] = true;
}

template<typename T, std::size_t N> void BoxArray<T, N>::destroy(std::size_t i) {
	if (valid[i]) {
		valid[i] = false;
		buffer[i].~T();

		// By zeroing memory, we assure that future attempts to invoke virtual functions on the object will segfault when dereferencing the vtable pointer.
		std::memset(static_cast<void *>(&buffer[i]), 0, sizeof(T));
	}
}

#endif

