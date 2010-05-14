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
		// Gets the predicted position after the passage of time.
		//
		point future_position(double delta_time) const __attribute__((warn_unused_result));

		//
		// Gets the predicted orientation after the passage of time.
		//
		double future_orientation(double delta_time) const __attribute__((warn_unused_result));

		//
		// Gets the predicted linear velocity.
		//
		point est_velocity() const __attribute__((warn_unused_result));

		//
		// Gets the predicted angular velocity.
		//
		double est_avelocity() const __attribute__((warn_unused_result));

		//
		// Gets the predicted linear acceleration.
		//
		point est_acceleration() const __attribute__((warn_unused_result));

		//
		// Gets the predicted angular acceleration.
		//
		double est_aacceleration() const __attribute__((warn_unused_result));

		//
		// Pushes a new sample of position into the prediction engine.
		//
		void add_prediction_datum(const point &pos, double orientation, double delta_time);

		/**
		 * Clears the prediction engine so that it estimates zero acceleration
		 * and velocity. This is suitable to be used after a large step change
		 * in apparent position caused by a change to the coordinate
		 * transformation from physical to AI coordinates, such as when
		 * switching field ends.
		 * \param pos the current position of the object
		 * \param orientation the current orientation of the object
		 */
		void clear_prediction(const point &pos, double orientation);

	private:
		ap::real_1d_array xhistory, yhistory, thistory;
		ap::real_1d_array dhistory; // history vector of time steps
		ap::real_1d_array weights;
		ap::real_2d_array fmatrix;
		ap::real_1d_array approxx, approxy, approxt;

		// Updates matrices used to build the least squares prediction.
		// Call whenever the history vectors are updated.
		void build_matrices();
};

#endif

