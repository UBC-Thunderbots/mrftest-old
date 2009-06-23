#ifndef DATAPOOL_PREDICTABLEOBJECT_H
#define DATAPOOL_PREDICTABLEOBJECT_H

#include "datapool/Object.h"
#include "datapool/Updateable.h"
#include "../src/leastsquares/ap.h"

class PredictableObject : public Object, public Updateable {
public:
	/*
	 * Updates the object (called at the fixed AI frame rate).
	 */
	void update();

	/*
	 * The predicted velocity of the object.
	 */
	const Vector2 &predictedVelocity() const {
		return predVel;
	}

	/*
	 * The predicted acceleration of the object.
	 */
	const Vector2 &predictedAcceleration() const {
		return predAcc;
	}

	/*
	 * Predicts a future position.
	 */
	Vector2 futurePosition(unsigned int timeOffset);

protected:
	/*
	 * Creates a new predictable object.
	 */
	PredictableObject();

private:
	ap::real_1d_array xhistory, yhistory;
	ap::real_1d_array weights;
	ap::real_2d_array fmatrix;
	ap::real_1d_array approxx, approxy;
	Vector2 predVel, predAcc;
};

#endif

