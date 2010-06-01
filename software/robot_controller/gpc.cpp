#include "gpc.h"
#include <algorithm>
#include <iostream>

namespace {

std::vector<double> conv(const std::vector<double> A,const std::vector<double> B) {
	std::vector<double> return_vector;
	
	if(B.size() > 0) {
		for(unsigned int index=0;index<A.size();index++)
			return_vector.push_back(B[0]*A[index]);
		
		for(unsigned int index=0;index<(B.size()-1);index++)
			return_vector.push_back(0);
			
		for(unsigned int index_b=1;index_b<B.size();index_b++)
			for(unsigned int index_a=0;index_a<A.size();index_a++)
				return_vector[index_b+index_a] += B[index_b]*A[index_a];
	}
	return return_vector;

}

const double initial_uncertainty = 10;

}


gpc::gpc(unsigned int numpoles,unsigned int numzeros,unsigned int N2,unsigned int N1) {
	this->N1=N1;
	this->N2=N2;
	
	num_zeros = numzeros;
	num_poles = numpoles;
	
	unsigned int covariance_size = numpoles + num_zeros + 1;
	covariance.SetSize(covariance_size,covariance_size);
	covariance.Null();
	
	for(unsigned int index=0;index<covariance_size;index++)
		covariance(index,index)=initial_uncertainty;
	
	
	double length = std::max(num_poles,num_zeros+1);
	for(unsigned int index=0;index<(length+N2);index++)
		prev_inputs.push_back(0);
	
	
	for(unsigned int index=0;index<(length+N2);index++)
		prev_outputs.push_back(0);
		
	
	theta.SetSize(num_zeros+num_poles+1,1);
	for(unsigned int index=0;index < num_zeros+num_poles+1;index++)
		theta(index,0)=0;
	theta(0,0)=1;	
}

gpc::~gpc() {
}



void gpc::update(double output) {
	current_output = output;
	math::matrix<double> regressor(num_zeros+num_poles+1,1);
	math::matrix<double> kalman_gain(num_zeros+num_poles+1,1);
	double scaling;
	
	for(unsigned int index=0;index<(num_zeros+1);index++)
		regressor(index,0) = prev_inputs[index];
	
	for(unsigned int index=0;index<num_poles;index++)
		regressor(num_zeros+1+index,0) = prev_outputs[index];
	
	scaling = (1 + ((~regressor)*covariance*regressor)(0,0));
	kalman_gain = covariance*regressor/scaling;
	covariance = covariance - (covariance*regressor*(~regressor)*covariance)/scaling;
	theta = theta + kalman_gain * (output - ((~regressor)*theta)(0,0));

}

void gpc::push_history(double input,double output) {
	prev_inputs.insert(prev_inputs.begin(), input);
	prev_inputs.pop_back();
	prev_outputs.insert(prev_outputs.begin(), output);
	prev_outputs.pop_back();
}	

void gpc::push_history(double input) {
	push_history(input,current_output);
}

void gpc::build_forward() {
	std::vector<double> delta;
	delta.push_back(1);
	delta.push_back(-1);
	
	std::vector<double> A;
	std::vector<double> B;
	A.push_back(1);
	
	std::vector<double> U_filtered = conv(prev_inputs,delta);
	
	for(unsigned int index=0;index<(num_zeros+1);index++)
		B.push_back(theta(index,0));
			
	for(unsigned int index=0;index<num_poles;index++)
		A.push_back(theta(num_zeros+1+index,0));
		
	std::vector<double> A_tild = conv(A,delta);
	std::vector<double> F(A_tild.begin()+1,A_tild.end());	
	std::vector<double> Eprime;
	Eprime.push_back(1);
	
	std::vector<double> F_s;
	
	//math::matrix<double> Gmat(Eprime.size()+B.size()-1,Eprime.size()+B.size()-1);
	math::matrix<double> Gmat;
	Gmat.SetSize(N2,N2);
	Gmat.Null();
	
	for(int index=0;index<static_cast<int>(N2);index++) {
		double r_j = F[0];
		std::vector<double> Ftemp;
		F.push_back(0);
		for(unsigned int index2=0;index2<(A_tild.size()-1);index2++)
			Ftemp.push_back(-F[index2+1]-r_j*A_tild[index2+1]);
		F=Ftemp;
		Eprime.push_back(r_j);
		std::vector<double> G = conv(Eprime,B);
		
		double temp;
		for(unsigned int index2 = 0;index2<F.size();index2++)
			temp = F[index2]*prev_outputs[index2];
		
		for(unsigned int index2 = 0;index2<(G.size()-index-1);index2++)
			temp += G[index+1+index2] * U_filtered[index2];
			
		F_s.push_back(temp);
		
		for(int index2 = 0; index2 < static_cast<int>(N2);index2++) {
			if(index - index2 >=0) {
				Gmat(index,index2) = G[index - index2];
			}
		}
	}
	
	free_response = F_s;
	std::vector<double> temp;
	for(unsigned int index =0; index < N2;index++)
		temp.push_back(Gmat(index,0));
	step_response = temp;	
}

double gpc::calc_control(double set_point) {
	build_forward();
	
	math::matrix<double> Gmat(N2-N1,1);
	math::matrix<double> Fs(N2-N1,1);
	math::matrix<double> Ysp(N2-N1,1);
	for(unsigned int index=0;index < (N2-N1);index++) {
		Gmat(index,0)=step_response[index];
		Fs(index,0)=free_response[index];
		Ysp(index,0)=set_point;
	}
	
#warning impliment lambda
	double scale = ((~Gmat)*Gmat)(0,0) + 1;
	
	return (~Gmat*(Ysp - Fs))(0,0)/scale;
}

const std::vector<double>& gpc::get_free_response()  const {
	return free_response;
}


const std::vector<double>& gpc::get_step_response() const {
	return step_response;
}

double gpc::get_numpoles() const {
	return num_poles;
}

double gpc::get_numzeros() const {
	return num_zeros;
}

std::vector<double> gpc::get_parameter_estimates() const {
	std::vector<double> returnval;
	
	for(unsigned int index = 0;index<num_poles+num_zeros+1;index++)
		returnval.push_back(theta(index,0));
	
	return returnval;
}
