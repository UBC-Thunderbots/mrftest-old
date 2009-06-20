#ifndef DATAPOOL_UPDATABLE_H
#define DATAPOOL_UPDATABLE_H

//
// An object that has an update() method to be called at a regular frequency.
//
class Updateable {
public:
	virtual void update() = 0;
};

#endif

