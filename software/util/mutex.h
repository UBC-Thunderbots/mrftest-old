#ifndef UTIL_MUTEX_H
#define UTIL_MUTEX_H

#include "util/noncopyable.h"
#include <pthread.h>

/**
 * Allows safe scope-bounded acquisition of a mutex.
 */
class MutexScopedAcquire : public NonCopyable {
	public:
		/**
		 * Acquires the mutex.
		 *
		 * \param[in] mtx the mutex to acquire.
		 */
		MutexScopedAcquire(pthread_mutex_t *mtx);

		/**
		 * Releases the mutex.
		 */
		~MutexScopedAcquire();

	private:
		pthread_mutex_t *const mutex;
};

#endif

