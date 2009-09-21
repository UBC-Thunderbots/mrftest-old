#ifndef UTIL_NONCOPYABLE_H
#define UTIL_NONCOPYABLE_H

//
// An object that should not be copied or assigned.
//
class noncopyable {
	protected:
		//
		// Noncopyable objects can still be constructed.
		//
		noncopyable() {
		}

	private:
		//
		// Prevents objects from being copied.
		//
		noncopyable(const noncopyable &copyref);

		//
		// Prevents objects from being assigned to one another.
		//
		noncopyable &operator=(const noncopyable &assgref);
};

#endif

