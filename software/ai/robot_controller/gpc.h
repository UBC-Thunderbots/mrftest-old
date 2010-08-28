#ifndef _GPC_H
#define _GPC_H

#include <cstring>
#include <vector>
#define __MINMAX_DEFINED
#include "util/matrix.h"

/**
 * An implementation of a SISO GPC Controller.
 */
class GPC {

	private: 
		/**
		 * The Covariance matrix for the internal recursive least squares.
		 */
		math::matrix<double> covariance;
		
		/**
		 * The storage vector for the parameters resulting from the least
		 * squares.
		 */
		math::matrix<double> theta;
		
		/**
		 * A vector of previous inputs to the system to be controlled.
		 */
		std::vector<double> prev_inputs;
		
		/**
		 * A vector of previous outputs from the system to be controlled.
		 */
		std::vector<double> prev_outputs;
		
		
		/**
		 * The number of zeros to include in the process model.
		 */
		unsigned int num_zeros;
		
		/**
		 * The number of poles to include in the process model.
		 */
		unsigned int num_poles;
		
		/**
		 * A vector containing the future reponse of the system if the control
		 * signal stays the same.
		 */
		std::vector<double> free_response;
		
		/**
		 * A vector containing the step response of the system.
		 */
		std::vector<double> step_response;
		
		/**
		 * The current output of the system for this iteration.
		 */
		double current_output;
		
		/**
		 * The minimum horizon.
		 */
		unsigned int N1;
		
		/**
		 * The maximum horizon.
		 */
		unsigned int N2;
		
		
	public:
		
		/**
		 * Creates a GPC controller.
		 *
		 * \param[in] numpoles the number of poles to approximate the system
		 * with.
		 *
		 * \param[in] numzeros the number of zeros to approximate the system
		 * with.
		 *
		 * \param[in] N2 the maximum horizon to look to.
		 *
		 * \param[in] N1 the mimimum horizon to look to.
		 */
		GPC(unsigned int numpoles=5, unsigned int numzeros=5,unsigned int N2=20,unsigned int N1=0);
		
		/**
		 * This is a "well dah".
		 */
		~GPC();
		
			
		/**
		 * After an update this method creates the free_response and
		 * step_response vectors. It is called in calc_control(double).
		 *
		 * \sa update(double)
		 */
		void build_forward();
		
		
		/**
		 * Used to advance the recursive estimator and get an new estimate of
		 * theta. This should be called first in an interation.
		 *
		 * \param[in] output current output of system to be controlled.
		 */
		void update(double output);
		
		/**
		 * Used to calculate the change in control action for this time step.
		 * This should be called after update(double) but before
		 * push_history(double, double).
		 *
		 * \param[in] set_point the desired setpoint for the Horizon.
		 *
		 * \return the increment to the control action that needs to be made.
		 *
		 * \sa update(double), push_history(double, double)
		 */
		double calc_control(double set_point);
		 
		/**
		 * Stores the history nessecry to compute the regresson as well as the
		 * look ahead prediction.
		 *
		 * \param[in] input the current control action just calculated.
		 * \param[in] output current output of the system to be controlled.
		 */
		void push_history(double input,double output);
		
		/**
		 * Stores the history necessary to compute the regression as well as the
		 * look ahead prediction, assumes previous output used in update.
		 *
		 * \param[in] input the current control action just calculated.
		 *
		 * \sa push_history(double, double);
		 */
		void push_history(double input);
		
		/**
		 * Gets the uncontrolled response of the system.
		 *
		 * \return vector containing the future response of the system.
		 */
		const std::vector<double>& get_free_response() const;
		
		/**
		 * Gets the Step response of the system.
		 *
		 * \return vector containing the step response of the system.
		 */
		const std::vector<double>& get_step_response() const;
		
		
		/**
		 * \return the current number of poles to approximate to.
		 */
		double get_numpoles() const;
		
		/**
		 * \return the current number of zeros to approximate to.
		 */
		double get_numzeros() const;

		/**
		 * \return the current theta vector.
		 */
		std::vector<double> get_parameter_estimates() const;

};

#endif
