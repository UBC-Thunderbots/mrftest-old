#include <cstring>
#include <vector>
#include "util/matrix.h"
/**
An impementation of the a SISO GPC Controller
*/
class gpc {

	private: 
		/// The Covariance matrix for the internal recursive least squares
		math::matrix<double> covariance;
		
		/// The storage vector for the parameters resulting from the least squares
		math::matrix<double> theta;
		
		/// A vector of previous inputs to the system to be controlled
		std::vector<double> prev_inputs;
		
		/// A vector of previous outputs from the system to be controlled
		std::vector<double> prev_outputs;
		
		
		/// The number of zeros to include in the process model
		unsigned int num_zeros;
		
		/// The number of poles to include in the process model
		unsigned int num_poles;
		
		/// A vector containing the future reponse of the system if the control signal stays the same
		std::vector<double> free_response;
		
		/// A veotor containing the step response of the system
		std::vector<double> step_response;
		
		/// store the current output of the system for this iteration
		double current_output;
		
		/// The minimum horizon
		unsigned int N1;
		
		/// The maximum horizon
		unsigned int N2;
		
	protected:
	
		/**
			after an update this method creates the free_response and step_response vectors
			it is called in calc_control()
			\sa update()
		*/
		void build_forward();
		
	public:
		
		/**
			Constructor for the GPC controller
			\param numpoles The number of poles to approximate the system with
			\param numzeros The number of zeros to approximate the system with
			\param N2 The maximum horizon to look to 
			\param N1 The mimimum horizon to look to
		*/
		gpc(unsigned int numpoles=5, unsigned int numzeros=5,unsigned int N2=20,unsigned int N1=0);
		
		/**
			This is a "well dah" 
		*/
		~gpc();
		
		/**
			Used to advance the recursive estimator and get an new estimate of theta
			This should be called first in an interation
			\param output current output of system to be controlled
		*/
		void update(double output);
		
		/**
			Used to calculate the change in control action for this time step
			This should be called after update() but before push_history()
			\param set_point the desired setpoint for the Horizon
			\return The increment to the control action that needs to be made
			\sa update(), push_history()
		*/
		double calc_control(double set_point);
		 
		 /**
			Stores the history nessecry to compute the regresson as well
			as the look ahead prediction
			\param output Current output of the system to be controlled
			\param input the current control action just calculated
			
		 */
		void push_history(double output,double input);
		
		/**
			Gets the uncontrolled response of the system
			\return vector containing the future reponse of the system
		*/
		const std::vector<double>& get_free_response() const;
		
		/**
			Gets the Step response of the system
			\return vector containing the step response of the system
		*/
		const std::vector<double>& get_step_response() const;
		
		
		/**
			\return the current number of poles to approximate to
		*/
		double get_numpoles() const;
		
		/**
			\return the current number of zeros to approximate to
		*/
		double get_numzeros() const;
};
