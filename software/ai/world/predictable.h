#ifndef AI_WORLD_PREDICTABLE_H
#define AI_WORLD_PREDICTABLE_H

#include "geom/point.h"
#include "leastsquares/ap.h"
#include "util/time.h"

/**
 * An object whose velocity can be predicted based on a collection of prior positions.
 */
class Predictable {
	public:
		/**
		 * Constructs a new Predictable object.
		 */
		Predictable();

		/**
		 * Gets the predicted current position.
		 *
		 * \return the predicted position at the instant lock_time() was last called.
		 */
		Point position() const __attribute__((warn_unused_result)) {
			return position_;
		}

		/**
		 * Gets the predicted current orientation.
		 *
		 * \return the predicted orientation at the instant lock_time() was last called.
		 */
		double orientation() const __attribute__((warn_unused_result)) {
			return orientation_;
		}

		/**
		 * Gets a predicted future position.
		 *
		 * \return the predicted position \p delta_time after the most recent invocation of add_prediction_datum(const Point &, double).
		 */
		Point future_position(double delta_time) const __attribute__((warn_unused_result));

		/**
		 * Gets a predicted future orientation.
		 *
		 * \return the predicted orientation \p delta_time after the most recent invocation of add_prediction_datum(const Point &, double).
		 */
		double future_orientation(double delta_time) const __attribute__((warn_unused_result));

		/**
		 * Gets the predicted current velocity.
		 *
		 * \return the predicted linear velocity in metres per second at the time of the most recent invocation of
		 * add_prediction_datum(const Point &, double).
		 */
		Point est_velocity() const __attribute__((warn_unused_result));

		/**
		 * Gets the predicted current angular velocity.
		 *
		 * \return the predicted angular velocity in radians per second at the time of the most recent invocation of
		 * add_prediction_datum(const Point &, double).
		 */
		double est_avelocity() const __attribute__((warn_unused_result));

		/**
		 * Gets the predicted linear acceleration.
		 *
		 * \return the predicted linear acceleration in metres per second squared at the time of the most recent invocation of
		 * add_prediction_datum(const Point &, double).
		 */
		Point est_acceleration() const __attribute__((warn_unused_result));

		/**
		 * Gets the predicted angular acceleration.
		 *
		 * \return the predicted angular acceleration in radians per second squared at the time of the most recent invocation of
		 * add_prediction_datum(const Point &, double).
		 */
		double est_aacceleration() const __attribute__((warn_unused_result));

	protected:
		/**
		 * Pushes a new sample of position into the prediction engine.
		 *
		 * \param[in] pos the new position of the object.
		 *
		 * \param[in] orientation the new orientation of the object.
		 */
		void add_prediction_datum(const Point &pos, double orientation);

		/**
		 * Clears the prediction engine so that it estimates zero acceleration and velocity.
		 * This is suitable to be used after a large step change in apparent position.
		 * Such a change might be caused by a change to the coordinate transformation from physical to %AI coordinates.
		 * This might happen when switching field ends.
		 *
		 * \param[in] pos the current position of the object.
		 *
		 * \param[in] orientation the current orientation of the object.
		 */
		void clear_prediction(const Point &pos, double orientation);

		/**
		 * Captures the current instant as the time for which position() and orientation() should return values.
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
		Point position_;
		double orientation_;

		// Updates matrices used to build the least squares prediction.
		// Call whenever the history vectors are updated.
		void build_matrices();
};

#endif

