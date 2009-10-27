#ifndef WORLD_PREDICTABLE_H
#define WORLD_PREDICTABLE_H

#include "geom/point.h"
#include "leastsquares/ap.h"

//
// An object whose velocity can be predicted based on a collection of prior positions.
//
class predictable {
	public:
		//
		// Constructs a new predictable object.
		//
		predictable();

		//
		// Gets the predicted velocity.
		//
		point velocity() const;

	protected:
		//
		// Pushes a new sample of position into the prediction engine.
		//
		void add_position(const point &pos);

	private:
		ap::real_1d_array xhistory, yhistory;
		ap::real_1d_array weights;
		ap::real_2d_array fmatrix;
		ap::real_1d_array approxx, approxy;
};

#endif

