#ifndef AI_WORLD_PREDICTABLE_H
#define AI_WORLD_PREDICTABLE_H

#include "geom/point.h"
#include "leastsquares/ap.h"
#include "util/time.h"

/**
 * An object whose velocity can be predicted based on a collection of prior positions.
 */
class predictable {
	public:
		/**
		 * Constructs a new predictable object.
		 */
		predictable();

		/**
		 * \return The predicted position at the instant lock_time() was last
		 * called
		 */
		point position() const __attribute__((warn_unused_result)) {
			return position_;
		}

		/**
		 * \return The predicted orientation at the instant lock_time() was last
		 * called
		 */
		double orientation() const __attribute__((warn_unused_result)) {
			return orientation_;
		}

		/**
		 * \return The predicted position delta_time after the most recent
		 * invocation of add_prediction_datum
		 */
		point future_position(double delta_time) const __attribute__((warn_unused_result));

		/**
		 * \return The predicted orientation delta_time after the most recent
		 * invocation of add_prediction_datum
		 */
		double future_orientation(double delta_time) const __attribute__((warn_unused_result));

		/**
		 * \return The predicted linear velocity
		 */
		point est_velocity() const __attribute__((warn_unused_result));

		/**
		 * \return The predicted angular velocity
		 */
		double est_avelocity() const __attribute__((warn_unused_result));

		/**
		 * \return The predicted linear acceleration
		 */
		point est_acceleration() const __attribute__((warn_unused_result));

		/**
		 * \return The predicted angular acceleration
		 */
		double est_aacceleration() const __attribute__((warn_unused_result));

	protected:
		/**
		 * Pushes a new sample of position into the prediction engine.
		 * \param pos the new position of the object
		 * \param orientation the new orientation of the object
		 */
		void add_prediction_datum(const point &pos, double orientation);

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

		/**
		 * Captures the current instant as the time for which position() and
		 * orientation() should return values.
		 */
		void lock_time();

	private:
		bool initialized;
		ap::real_1d_array xhistory, yhistory, thistory;
		ap::real_1d_array dhistory; // history vector of time steps
		ap::real_1d_array weights;
		ap::real_2d_array fmatrix;
		ap::real_1d_array approxx, approxy, approxt;
		timespec last_datum_timestamp;
		point position_;
		double orientation_;

		// Updates matrices used to build the least squares prediction.
		// Call whenever the history vectors are updated.
		void build_matrices();
};

#endif

