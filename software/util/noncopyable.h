#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

//
// Prevents copying of objects.
//
class noncopyable {
	private:
		noncopyable(const noncopyable &copyref);
		noncopyable &operator=(const noncopyable &assgref);
};

#endif

