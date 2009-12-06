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
		point est_velocity() const __attribute__((warn_unused_result));

		//
		// Gets the predicted angular velocity.
		//
		double est_avelocity() const __attribute__((warn_unused_result));

		//
		// Pushes a new sample of position into the prediction engine.
		//
		void add_prediction_datum(const point &pos, double orientation);

	private:
		ap::real_1d_array xhistory, yhistory, thistory;
		ap::real_1d_array weights;
		ap::real_2d_array fmatrix;
		ap::real_1d_array approxx, approxy, approxt;
};

#endif

