#ifndef TESTER_ZEROABLE_H
#define TESTER_ZEROABLE_H

//
// An object that can be set to zero.
//
class zeroable {
	public:
		//
		// Zeroes the object.
		//
		virtual void zero() = 0;
};

#endif

