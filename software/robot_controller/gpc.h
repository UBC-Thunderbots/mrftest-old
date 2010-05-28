#include <cstring>
#include <vector>
#include "util/matrix.h"

class gpc {
	private: 
		math::matrix<double> covariance;
		math::matrix<double> theta;
		std::vector<double> prev_inputs;
		std::vector<double> prev_outputs;
		unsigned int num_zeros;
		unsigned int num_poles;
		std::vector<double> free_response;
		std::vector<double> step_response;
		double current_output;
		unsigned int N1;
		unsigned int N2;
		
	protected:
		void build_forward();
		
	public:
		
		
		gpc(unsigned int numpoles=5, unsigned int numzeros=5,unsigned int N2=20,unsigned int N1=0);
		~gpc();
		void update(double output);
		void push_history(double output,double input);
		double calc_control(double set_point);
		const std::vector<double>& get_free_response() const;
		const std::vector<double>& get_step_response() const;
		double get_numpoles() const;
		double get_numzeros() const;
		
};
