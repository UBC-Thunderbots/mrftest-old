#ifndef DATAPOOL_PREDICTABLEOBJECT_H
#define DATAPOOL_PREDICTABLEOBJECT_H

#include "datapool/Object.h"
#include "../src/leastsquares/ap.h"

class PredictableObject : public Object {
public:
	/*
	 * Sets the position of the object.
	 */
	virtual void position(const Vector2 &pos);
	using Object::position;

	/*
	 * Predicts a future position.
	 */
	Vector2 futurePosition(unsigned int timeOffset);

protected:
	/*
	 * Creates a new predictable object.
	 */
	PredictableObject();

	/*
	 * Destroys the object.
	 */
	virtual ~PredictableObject();

private:
	PredictableObject(const PredictableObject &copyref); // Prohibit copying.
	ap::real_1d_array xhistory, yhistory;
	ap::real_1d_array weights;
	ap::real_2d_array fmatrix;
	ap::real_1d_array approxx, approxy;
};

#endif

