//Object:  VIRTUAL BASE CLASS
//CONTAINS ALL BASIC FUNCTIONS THAT BALL AND PLAYER CLASSES SHARE
#ifndef DATAPOOL_OBJECT_H
#define DATAPOOL_OBJECT_H

#include "datapool/Vector2.h"

class Object {
public:
	void radius(double r);
	double radius() const;

	virtual void position(const Vector2 &pos);
	const Vector2 &position() const;

	void velocity(const Vector2 &vel);
	const Vector2 &velocity() const;

	void acceleration(const Vector2 &acc);
	const Vector2 &acceleration() const;

protected:
	/*
	 * Creates a new object.
	 */
	Object();

	/*
	 * Destroys the object.
	 */
	virtual ~Object();

private:
	Object(const Object &copyref); // Prohibit copying.

	double r; //assuming all objects are circular

	Vector2 pos;
	Vector2 vel;
	Vector2 acc;
};

#endif

