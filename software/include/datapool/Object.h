//Object:  VIRTUAL BASE CLASS
//CONTAINS ALL BASIC FUNCTIONS THAT BALL AND PLAYER CLASSES SHARE
#ifndef DATAPOOL_OBJECT_H
#define DATAPOOL_OBJECT_H

#include "datapool/Noncopyable.h"
#include "datapool/Vector2.h"

class Object : public virtual Noncopyable {
public:
	/*
	 * The radius of the (assumed-circular) object.
	 */
	void radius(double r) {
		rad = r;
	}
	double radius() const {
		return rad;
	}

	/*
	 * The position of the object.
	 */
	void position(const Vector2 &p) {
		pos = p;
	}
	const Vector2 &position() const {
		return pos;
	}

	/*
	 * The velocity the object is moving, according to the simulator. Not used in IR mode.
	 */
	void simulatedVelocity(const Vector2 &v) {
		simVel = v;
	}
	const Vector2 &simulatedVelocity() const {
		return simVel;
	}

	/*
	 * The acceleration of the object, according to the simulator. Not used in IR mode.
	 */
	void simulatedAcceleration(const Vector2 &a) {
		simAcc = a;
	}
	const Vector2 &simulatedAcceleration() const {
		return simAcc;
	}

protected:
	/*
	 * Creates a new object.
	 */
	Object() : rad(0), pos(), simVel(), simAcc() {
	}

private:
	double rad; //assuming all objects are circular

	Vector2 pos;
	Vector2 simVel;
	Vector2 simAcc;
};

#endif

