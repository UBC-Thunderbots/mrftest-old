#ifndef UTIL_ASYNC_OPERATION_H
#define UTIL_ASYNC_OPERATION_H

#include "util/byref.h"
#include <sigc++/sigc++.h>

/**
 * \brief An asynchronous operation which is currently in progress.
 *
 * \tparam T the return type of the operation.
 */
template<typename T> class AsyncOperation : public ByRef {
	public:
		/**
		 * \brief A pointer to an AsyncOperation.
		 */
		typedef RefPtr<AsyncOperation> Ptr;

		/**
		 * \brief Invoked when the operation completes, whether successfully or not.
		 *
		 * The callback functions are passed the operation itself.
		 */
		sigc::signal<void, Ptr> signal_done;

		/**
		 * \brief Checks for the success of the operation and returns the return value.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 *
		 * \return the return value.
		 */
		virtual T result() const = 0;

		/**
		 * \brief Checks whether or not the operation failed.
		 *
		 * The default implementation calls result() and checks whether it throws an exception.
		 *
		 * \return \c true if the operation failed, or \c false if not.
		 */
		virtual bool succeeded() const {
			try {
				result();
				return true;
			} catch (...) {
				return false;
			}
		}

	protected:
		/**
		 * \brief Constructs a new AsyncOperation.
		 */
		AsyncOperation() {
		}

		/**
		 * \brief Destroys an AsyncOperation.
		 */
		~AsyncOperation() {
		}
};

#endif

