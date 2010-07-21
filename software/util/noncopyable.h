#ifndef UTIL_NONCOPYABLE_H
#define UTIL_NONCOPYABLE_H

/**
 * An object that should not be copied or assigned.
 */
class NonCopyable {
	protected:
		/**
		 * Noncopyable objects can still be constructed.
		 */
		NonCopyable() {
		}

	private:
		/**
		 * Prevents objects from being copied.
		 */
		NonCopyable(const NonCopyable &);

		/**
		 * Prevents objects from being assigned to one another.
		 */
		NonCopyable &operator=(const NonCopyable &);
};

#endif

