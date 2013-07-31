#ifndef UTIL_BOX_PTR_H
#define UTIL_BOX_PTR_H

#include <cassert>
#include <cstddef>
#include <functional>

template<typename T> class BoxPtr;
template<typename T> class Box;

namespace std {
	/**
	 * \brief Provides a total ordering of boxed pointers so they can be stored in STL ordered containers.
	 *
	 * \tparam T the type of object pointed to.
	 */
	template<typename T> struct less<BoxPtr<T>> {
		public:
			/**
			 * \brief Compares two pointers.
			 *
			 * \param[in] x the first pointer.
			 *
			 * \param[in] y the second pointer.
			 *
			 * \return \c true if \p x should precede \p y in an ordered container, or \c false if not.
			 */
			bool operator()(const BoxPtr<T> &x, const BoxPtr<T> &y) const;

		private:
			std::less<T *> cmp;
	};
}

/**
 * \brief Checks two pointers for equality.
 *
 * \param[in] x the first pointer.
 *
 * \param[in] y the second pointer.
 *
 * \return \c true if the pointers point to the same box, or \c false if not.
 */
template<typename T> bool operator==(const BoxPtr<T> &x, const BoxPtr<T> &y);

/**
 * \brief Checks two pointers for equality.
 *
 * \param[in] x the first pointer.
 *
 * \param[in] y the second pointer.
 *
 * \return \c false if the pointers point to the same box, or \c true if not.
 */
template<typename T> bool operator!=(const BoxPtr<T> &x, const BoxPtr<T> &y);

/**
 * \brief A pointer to a boxed object.
 *
 * Unlike normal pointers which must not be used following destruction of the pointed-to object, a box pointer is safe to use following destruction of its target object.
 * However, a box pointer must not be used following the destruction of the associated box.
 *
 * \tparam T the type of object.
 */
template<typename T> class BoxPtr {
	public:
		/**
		 * \brief Constructs a new, null pointer.
		 */
		BoxPtr();

		/**
		 * \brief Copies an existing pointer.
		 */
		template<typename U> BoxPtr(const BoxPtr<U> &copyref);

		/**
		 * \brief Assigns a pointer to this pointer.
		 *
		 * \param[in] assgref the pointer to assign.
		 *
		 * \return this pointer.
		 */
		BoxPtr<T> &operator=(const BoxPtr<T> &assgref);

		/**
		 * \brief Dereferences the pointer.
		 *
		 * \return the underlying object.
		 */
		T &operator*() const;

		/**
		 * \brief Dereferences the pointer.
		 *
		 * \return the underlying pointer.
		 */
		T *operator->() const;

		/**
		 * \brief Sets the pointer to null.
		 */
		void reset();

		/**
		 * \brief Checks whether the pointer points to a valid object.
		 *
		 * \return \c true if the pointer points to a valid object, or \c false if the pointer is null or points to an invalid object.
		 */
		explicit operator bool() const;

	private:
		T *ptr;
		const bool *valid;

		BoxPtr(T *p, const bool *valid);

		template<typename U> friend class BoxPtr;
		template<typename U> friend class Box;
		friend struct std::less<BoxPtr<T>>;
		friend bool operator==<>(const BoxPtr<T> &, const BoxPtr<T> &);
		friend bool operator!=<>(const BoxPtr<T> &, const BoxPtr<T> &);
};



template<typename T> bool std::less<BoxPtr<T>>::operator()(const BoxPtr<T> &x, const BoxPtr<T> &y) const {
	return cmp(x.ptr, y.ptr);
}

template<typename T> bool operator==(const BoxPtr<T> &x, const BoxPtr<T> &y) {
	return x.ptr == y.ptr;
}

template<typename T> bool operator!=(const BoxPtr<T> &x, const BoxPtr<T> &y) {
	return x.ptr != y.ptr;
}

template<typename T> BoxPtr<T>::BoxPtr() : ptr(0), valid(0) {
}

template<typename T> template<typename U> BoxPtr<T>::BoxPtr(const BoxPtr<U> &copyref) : ptr(copyref.ptr), valid(copyref.valid) {
}

template<typename T> BoxPtr<T> &BoxPtr<T>::operator=(const BoxPtr<T> &assgref) {
	ptr = assgref.ptr;
	valid = assgref.valid;
	return *this;
}

template<typename T> T &BoxPtr<T>::operator*() const {
	return *ptr;
}

template<typename T> T *BoxPtr<T>::operator->() const {
	return ptr;
}

template<typename T> void BoxPtr<T>::reset() {
	ptr = 0;
	valid = 0;
}

template<typename T> BoxPtr<T>::operator bool() const {
	return valid ? *valid : false;
}

template<typename T> BoxPtr<T>::BoxPtr(T *p, const bool *valid) : ptr(p), valid(valid) {
}

#endif

