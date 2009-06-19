#ifndef DATAPOOL_NONCOPYABLE_H
#define DATAPOOL_NONCOPYABLE_H

//
// A class extending this class cannot be copied (i.e. it cannot be copy-constructed or assigned).
// This is useful for classes that hold onto resources of which copies should not be taken.
//
class Noncopyable {
protected:
	Noncopyable() {
	}

	~Noncopyable() {
	}

private:
	// These functions are private and have no actual implementations anywhere in the code.
	// This prevents them from ever being called.
	Noncopyable(const Noncopyable &copyref);
	Noncopyable &operator=(const Noncopyable &assgref);
};

#endif

