#include "util/exception.h"
#include "util/umask.h"
#include <pthread.h>
#include <sys/stat.h>

namespace {
	pthread_mutex_t mutex;
	pthread_once_t once = PTHREAD_ONCE_INIT;

	void init_mutex_impl() {
		pthread_mutexattr_t attr;
		int rc;

		if ((rc = pthread_mutexattr_init(&attr)) != 0) {
			throw SystemError("pthread_mutexattr_init", rc);
		}

		if ((rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)) != 0) {
			pthread_mutexattr_destroy(&attr);
			throw SystemError("pthread_mutexattr_settype", rc);
		}

		if ((rc = pthread_mutex_init(&mutex, &attr)) != 0) {
			pthread_mutexattr_destroy(&attr);
			throw SystemError("pthread_mutex_init", rc);
		}

		pthread_mutexattr_destroy(&attr);
	}

	pthread_mutex_t *get_mutex() {
		int rc = pthread_once(&once, &init_mutex_impl);
		if (rc != 0) {
			throw SystemError("pthread_once", rc);
		}
		return &mutex;
	}
}

UMaskScopedModification::UMaskScopedModification(mode_t new_mode) : mutex_acquisition(get_mutex()), old_mode(umask(new_mode)) {
}

UMaskScopedModification::~UMaskScopedModification() {
	umask(old_mode);
}

