#ifndef UTIL_BYREF_H
#define UTIL_BYREF_H

#include "util/noncopyable.h"
#include <utility>

/**
 * A pointer that performs reference counting on its referent.
 *
 * \tparam T the type of object to which the pointer points.
 */
template<typename T> class RefPtr {
	public:
		/**
		 * Performs a \c static_cast on a RefPtr.
		 *
		 * \tparam U the type to \c static_cast from.
		 *
		 * \param[in] pu the pointer to cast.
		 *
		 * \return a RefPtr pointing to the result of performing a \c static_cast<T> on <code>pu</code>'s underlying pointer.
		 */
		template<typename U> static RefPtr<T> cast_static(const RefPtr<U> &pu) {
			T *pt = static_cast<T *>(pu.operator->());
			RefPtr<T> p(pt);
			return p;
		}

		/**
		 * Performs a \c dynamic_cast on a RefPtr.
		 *
		 * \tparam U the type to \c dynamic_cast from.
		 *
		 * \param[in] pu the pointer to cast.
		 *
		 * \return a RefPtr pointing to the result of performing a \c dynamic_cast<T> on <code>pu</code>'s underlying pointer.
		 */
		template<typename U> static RefPtr<T> cast_dynamic(const RefPtr<U> &pu) {
			T *pt = dynamic_cast<T *>(pu.operator->());
			RefPtr<T> p(pt);
			return p;
		}

		/**
		 * Constructs a new null RefPtr.
		 */
		RefPtr() : obj(0) {
		}

		/**
		 * Constructs a new RefPtr by taking ownership of an object.
		 * The object's reference count is incremented.
		 *
		 * \param[in] p a pointer to the object to take ownership of, or a null pointer to create a null RefPtr.
		 */
		explicit RefPtr(T *p) : obj(p) {
			if (obj) {
				obj->reference();
			}
		}

		/**
		 * Copies a RefPtr.
		 *
		 * \param[in] copyref the RefPtr to copy.
		 */
		RefPtr(const RefPtr<T> &copyref) : obj(copyref.obj) {
			if (obj) {
				obj->reference();
			}
		}

		/**
		 * Converts a RefPtr pointing to a derived type into a RefPtr pointing to a base type.
		 *
		 * \tparam U the derived type.
		 *
		 * \param[in] copyref the derived RefPtr to convert.
		 */
		template<typename U> RefPtr(const RefPtr<U> &copyref) : obj(copyref.operator->()) {
			if (obj) {
				obj->reference();
			}
		}

		/**
		 * Destroys a RefPtr.
		 */
		~RefPtr() {
			reset();
		}

		/**
		 * Assigns a new RefPtr to this RefPtr.
		 *
		 * \param[in] assgref the RefPtr to assign.
		 *
		 * \return this RefPtr.
		 */
		RefPtr &operator=(const RefPtr<T> &assgref) {
			RefPtr<T> temp(assgref);
			swap(temp);
			return *this;
		}

		/**
		 * Transforms this RefPtr into a null RefPtr.
		 */
		void reset() {
			if (obj) {
				obj->unreference();
				obj = 0;
			}
		}

		/**
		 * Takes ownership of a new object.
		 * The object's reference count is incremented.
		 *
		 * \param[in] p a pointer to the object to take ownership of, or a null pointer to turn this RefPtr into a null RefPtr.
		 */
		void reset(T *p) {
			RefPtr<T> temp(p);
			swap(temp);
		}

		/**
		 * Invokes a function or accesses a variable in the object pointed to by this RefPtr.
		 *
		 * \return a pointer to the pointed-to object.
		 */
		T *operator->() const {
			return obj;
		}

		/**
		 * Compares two RefPtrs.
		 * Two RefPtrs are considered equal if they point at the same object or if they are both null.
		 *
		 * \param[in] other the RefPtr to compare to.
		 *
		 * \return \c true if \c this and \p other are equal, or \c false if not.
		 */
		bool operator==(const RefPtr<T> &other) const {
			return obj == other.obj;
		}

		/**
		 * Compares two RefPtrs.
		 * Two RefPtrs are considered equal if they point at the same object or if they are both null.
		 *
		 * \param[in] other the RefPtr to compare to.
		 *
		 * \return \c true if \c this and \p other are unequal, or \c false if not.
		 */
		bool operator!=(const RefPtr<T> &other) const {
			return obj != other.obj;
		}

		/**
		 * Checks whether this RefPtr is null or not.
		 *
		 * \return \c true if this RefPtr points to an object, or \c false if it is null.
		 */
		bool is() const {
			return obj != 0;
		}

		/**
		 * Swaps the contents of this RefPtr with another.
		 *
		 * \param[in] other the other RefPtr to swap with.
		 */
		void swap(RefPtr<T> &other) {
			std::swap(obj, other.obj);
		}

	private:
		T *obj;
};

namespace std {
	/**
	 * Swaps two \ref RefPtr "RefPtrs".
	 * Note that this is an override introduced into the \c std namespace and not a template specialization and is therefore technically illegal;
	 * that said, there exists no way to specialize \c std::swap on a template type, so this is the best solution.
	 *
	 * \tparam T the type to which the \ref RefPtr "RefPtrs" point.
	 *
	 * \param[in] x the first RefPtr to swap.
	 *
	 * \param[in] y the second RefPtr to swap.
	 */
	template<typename T> void swap(RefPtr<T> &x, RefPtr<T> &y) {
		x.swap(y);
	}
}

/**
 * An object that should be passed around by means of a RefPtr<> rather than by copying.
 */
class ByRef : public NonCopyable {
	public:
		/**
		 * Returns the reference count of the object.
		 * This can be used to check for leaking references at the expected point of destruction.
		 *
		 * \return the reference count.
		 */
		unsigned int refs() const {
			return refs_;
		}

	protected:
		/**
		 * Constructs a new ByRef.
		 * The object is assumed to have zero references,
		 * but is not deleted until its reference count becomes nonzero and then becomes zero again.
		 */
		ByRef() : refs_(0) {
		}

		/**
		 * Destroys a ByRef.
		 * This is here even though it doesn't do anything because it forces destructors all the way down the inheritance hierarchy to be virtual,
		 * which ensures that when a reference-counted object loses its last pointer,
		 * the <code>delete this</code> in unreference() invokes the correct destructor.
		 */
		virtual ~ByRef() {
		}

	private:
		/**
		 * The reference count of the object.
		 */
		unsigned int refs_;

		/**
		 * Adds one to the object's reference count.
		 */
		void reference() {
			++refs_;
		}

		/**
		 * Subtracts one from the object's reference count.
		 */
		void unreference() {
			if (!--refs_) {
				delete this;
			}
		}

		template<typename T> friend class RefPtr;
};

#endif

