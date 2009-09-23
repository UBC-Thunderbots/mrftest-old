#ifndef UTIL_MUTEX_H
#define UTIL_MUTEX_H

#include "util/noncopyable.h"
#include <pthread.h>

//
// A mutex.
//
class mutex : public virtual noncopyable {
	public:
		//
		// Constructs a new, initially-unlocked mutex.
		//
		mutex() {
			pthread_mutex_init(&mtx, 0);
		}

		//
		// Destroys a mutex.
		//
		~mutex() {
			pthread_mutex_destroy(&mtx);
		}

	private:
		pthread_mutex_t mtx;
		friend class mutex_scoped_lock;
};

//
// A scope-limited acquisition of a mutex.
//
class mutex_scoped_lock : public virtual noncopyable {
	public:
		//
		// Acquires a mutex.
		//
		mutex_scoped_lock(mutex &mtx) : mtx(mtx) {
			pthread_mutex_lock(&mtx.mtx);
		}

		//
		// Releases the mutex.
		//
		~mutex_scoped_lock() {
			pthread_mutex_unlock(&mtx.mtx);
		}

	private:
		mutex &mtx;
};

#endif

