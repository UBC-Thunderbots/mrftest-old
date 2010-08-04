#include "util/exception.h"
#include "util/mutex.h"

MutexScopedAcquire::MutexScopedAcquire(pthread_mutex_t *mtx) : mutex(mtx) {
	int rc = pthread_mutex_lock(mtx);
	if (rc != 0) {
		throw SystemError("pthread_mutex_lock", rc);
	}
}

MutexScopedAcquire::~MutexScopedAcquire() {
	pthread_mutex_unlock(mutex);
}

