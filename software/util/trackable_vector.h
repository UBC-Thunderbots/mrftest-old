#ifndef UTIL_TRACKABLE_VECTOR_H
#define UTIL_TRACKABLE_VECTOR_H

#include "util/noncopyable.h"
#include <vector>
#include <sigc++/sigc++.h>

namespace Util {
	/**
	 * A data structure that permits random access by index to its contents and sends signals when elements are added or removed.
	 *
	 * \tparam T the type of object stored in the TrackableVector.
	 */
	template<typename T>
	class TrackableVector : public NonCopyable {
		public:
			/**
			 * The type of object stored in the TrackableVector.
			 */
			typedef T value_type;

			/**
			 * The type of a reference to an object stored in the TrackableVector.
			 */
			typedef T &reference;

			/**
			 * The type of a reference to an object stored in the TrackableVector.
			 */
			typedef T &const_reference;

			/**
			 * The type of a pointer to an object stored in the TrackableVector.
			 */
			typedef T *pointer;

			/**
			 * The type of an iterator over a TrackableVector.
			 */
			typedef std::vector<T>::iterator iterator;

			/**
			 * The type of an iterator over a TrackableVector.
			 */
			typedef std::vector<T>::const_iterator const_iterator;

			/**
			 * The type of an iterator over a TrackableVector.
			 */
			typedef std::vector<T>::reverse_iterator reverse_iterator;

			/**
			 * The type of an iterator over a TrackableVector.
			 */
			typedef std::vector<T>::const_reverse_iterator const_reverse_iterator;

			/**
			 * The type of the difference between two iterators.
			 */
			typedef std::vector<T>::difference_type difference_type;

			/**
			 * The type of the size of the container.
			 */
			typedef std::vector<T>::size_type size_type;

			/**
			 * A signal fired whenever an element is added to this TrackableVector.
			 */
			mutable sigc::signal<void, size_type> signal_element_added;

			/**
			 * A signal fired whenever an element is removed from this TrackableVector.
			 */
			mutable sigc::signal<void, size_type> signal_element_removed;

			/**
			 * Constructs a new, empty TrackableVector.
			 */
			TrackableVector() {
			}

			/**
			 * Constructs a new TrackableVector containing all the elements from a sequence.
			 *
			 * \tparam Titer the type of the iterator across the original sequence.
			 *
			 * \param[in] i the beginning of the range to copy.
			 *
			 * \param[in] j the end of the range to copy.
			 */
			template<typename Titer> TrackableVector(Titer i, Titer j) : data(i, j) {
			}

			/**
			 * Constructs a new TrackableVector containing copies of an object.
			 *
			 * \param[in] n the number of copies to make.
			 *
			 * \param[in] t the object to copy.
			 */
			TrackableVector(size_type n, const T &t) : data(n, t) {
			}

			/**
			 * Destroys a TrackableVector.
			 */
			~TrackableVector() {
			}

			/**
			 * Checks if two TrackableVectors are equal.
			 *
			 * \param[in] other the other TrackableVector to compare to.
			 *
			 * \return \c true if both are equal, or \c false if not.
			 */
			bool operator==(const TrackableVector<T> &other) {
				return data == other.data;
			}

			/**
			 * Checks if two TrackableVectors are unequal.
			 *
			 * \param[in] other the other TrackableVector to compare to.
			 *
			 * \return \c true if they are unequal, or \c false if not.
			 */
			bool operator!=(const TrackableVector<T> &other) {
				return data != other.data;
			}

			/**
			 * Returns an iterator to the beginning of the TrackableVector.
			 *
			 * \return an iterator to the beginning of the TrackableVector.
			 */
			iterator begin() {
				return data.begin();
			}

			/**
			 * Returns an iterator to the beginning of the TrackableVector.
			 *
			 * \return an iterator to the beginning of the TrackableVector.
			 */
			const_iterator begin() const {
				return data.begin();
			}

			/**
			 * Returns an iterator to the end of the TrackableVector.
			 *
			 * \return an iterator to the end of the TrackableVector.
			 */
			iterator end() {
				return data.end();
			}

			/**
			 * Returns an iterator to the end of the TrackableVector.
			 *
			 * \return an iterator to the end of the TrackableVector.
			 */
			const_iterator end() const {
				return data.end();
			}

			/**
			 * Returns an iterator to the beginning of the TrackableVector's reversed view.
			 *
			 * \return an iterator to the beginning of the TrackableVector's reversed view.
			 */
			reverse_iterator rbegin() {
				return data.rbegin();
			}

			/**
			 * Returns an iterator to the beginning of the TrackableVector's reversed view.
			 *
			 * \return an iterator to the beginning of the TrackableVector's reversed view.
			 */
			const_reverse_iterator rbegin() const {
				return data.rbegin();
			}

			/**
			 * Returns an iterator to the end of the TrackableVector's reversed view.
			 *
			 * \return an iterator to the end of the TrackableVector's reversed view.
			 */
			reverse_iterator rend() {
				return data.rend();
			}

			/**
			 * Returns an iterator to the end of the TrackableVector's reversed view.
			 *
			 * \return an iterator to the end of the TrackableVector's reversed view.
			 */
			const_reverse_iterator rend() const {
				return data.rend();
			}

			/**
			 * Returns the size of the TrackableVector.
			 *
			 * \return the size of the TrackableVector.
			 */
			size_type size() const {
				return data.size();
			}

			/**
			 * Returns the maximum size of the TrackableVector.
			 *
			 * \return the maximum size of the TrackableVector.
			 */
			size_type max_size() const {
				return data.max_size();
			}

			/**
			 * Checks whether the TrackableVector is empty.
			 *
			 * \return \c true if empty, or \c false if not.
			 */
			bool empty() const {
				return data.empty();
			}

			/**
			 * Returns the first element in the TrackableVector.
			 *
			 * \return the first element in the TrackableVector.
			 */
			reference front() {
				return data.front();
			}

			/**
			 * Returns the first element in the TrackableVector.
			 *
			 * \return the first element in the TrackableVector.
			 */
			const_reference front() const {
				return data.front();
			}

			/**
			 * Returns the last element in the TrackableVector.
			 *
			 * \return the last element in the TrackableVector.
			 */
			reference back() {
				return data.back();
			}

			/**
			 * Returns the last element in the TrackableVector.
			 *
			 * \return the last element in the TrackableVector.
			 */
			const_reference back() const {
				return data.back();
			}

			/**
			 * Inserts a new element into the TrackableVector.
			 *
			 * \param[in] p the position at which to insert.
			 *
			 * \param[in] t the element to insert.
			 *
			 * \return an iterator pointing at the new element.
			 */
			iterator insert(iterator p, const T &t) {
				iterator ret = data.insert(p, t);
				signal_element_added.emit(p - begin());
				return ret;
			}

			/**
			 * Removes an element from the TrackableVector.
			 *
			 * \param[in] p the position of the element to remove.
			 *
			 * \return an iterator pointing to the immediately following element.
			 */
			iterator erase(iterator p) {
				iterator ret = data.erase(p);
				signal_element_removed.emit(ret - begin());
				return ret;
			}

			/**
			 * Adds an element to the back of the TrackableVector.
			 *
			 * \param[in] t the element to add.
			 */
			void push_back(const T &t) {
				data.push_back(t);
				signal_element_added.emit(size() - 1);
			}

			/**
			 * Removes the element at the back of the TrackableVector.
			 */
			void pop_back() {
				data.pop_back();
				signal_element_removed.emit(size()) : }

			/**
			 * Returns the nth element in the TrackableVector.
			 *
			 * \param[in] n the position to access.
			 *
			 * \return the element at position \p n.
			 */
			T & operator[](const size_type n) {
				return data[n];
			}

			/**
			 * Returns the nth element in the TrackableVector.
			 *
			 * \param[in] n the position to access.
			 *
			 * \return the element at position \p n.
			 */
			const T &operator[](const size_type n) const {
				return data[n];
			}

		private:
			/**
			 * The contents of the TrackableVector.
			 */
			std::vector<T> data;
	};
}

#endif

