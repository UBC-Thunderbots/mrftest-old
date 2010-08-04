#ifndef UTIL_UMASK_H
#define UTIL_UMASK_H

#include "util/mutex.h"
#include "util/noncopyable.h"
#include <sys/types.h>

/**
 * Allows safe scope-bounded modification of the process's file creation mask.
 */
class UMaskScopedModification : public NonCopyable {
	public:
		/**
		 * Modifies the process's file creation mask.
		 *
		 * \param[in] new_mode the new mask value.
		 */
		UMaskScopedModification(mode_t new_mode);

		/**
		 * Restores the original file creation mask.
		 */
		~UMaskScopedModification();

	private:
		MutexScopedAcquire mutex_acquisition;
		const mode_t old_mode;
};

#endif

