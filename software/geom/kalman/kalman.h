#ifndef GEOM_KALMAN_KALMAN_H
#define GEOM_KALMAN_KALMAN_H

#include "util/matrix.h"
#include <deque>

/**
 * \brief Implements the basic mathematics of a Kalman filter.
 */
class Kalman {
	public:
		/**
		 * \brief Constructs a new Kalman filter.
		 *
		 * \param[in] angle \c true for angular semantics (which imply 2π=0), or \c false for linear semantics.
		 *
		 * \param[in] measure_std the standard deviation of measurements fed to update(double, timespec).
		 *
		 * \param[in] accel_std expected standard deviation of the random acceleration imposed on the object (jostling).
		 *
		 * \param[in] decay_time_constant rate of velocity decay.
		 */
		Kalman(bool angle, double measure_std, double accel_std, double decay_time_constant);

		/**
		 * \brief Predicts values at a given time.
		 *
		 * \param[in] prediction_time the time at which values should be extracted.
		 *
		 * \param[out] state_predict the matrix of predicted values.
		 *
		 * \param[out] p_predict the matrix of predicted covariances.
		 */
		void predict(timespec prediction_time, Matrix &state_predict, Matrix &p_predict) const;

		/**
		 * \brief Adds a measurement to the filter.
		 *
		 * \param[in] measurement the measured value.
		 *
		 * \param[in] measurement_time the time at which the measurement was taken.
		 */
		void update(double measurement, timespec measurement_time);

		/**
		 * \brief Adds a system control input to the filter.
		 *
		 * \param[in] input the control input, which must be a second derivative of the system's value.
		 *
		 * \param[in] input_time the time at which the control input was delivered to the system.
		 */
		void add_control(double input, timespec input_time);

	private:
		struct ControlInput {
			ControlInput(timespec t, double v);

			timespec time;
			double value;
		};

		timespec last_measurement_time;
		double last_control;
		double sigma_m;
		double sigma_a;
		double time_constant;
		std::deque<ControlInput> inputs;
		Matrix h;
		Matrix p;
		Matrix state_estimate;
		bool is_angle;

		void predict_step(double timestep, double control, Matrix &state_predict, Matrix &p_predict) const;
		Matrix gen_f_mat(double timestep) const;
		Matrix gen_q_mat(double timestep) const;
		Matrix gen_b_mat(double timestep) const;
};

#endif

