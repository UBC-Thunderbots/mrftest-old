
#include "passMainLoop.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/stp/gradient_approach/ratepass.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"

#include <cmath>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <iostream>

//Replace pow(n, 0.5) with sqrt(n) should be more optimal


//namespace Evaluation = AI::HL::STP::GradientApproach;
using namespace AI::HL::W;

namespace AI {
namespace HL {
namespace STP {
namespace GradientApproach {
	std::vector<double> optimizePass(PassInfo::worldSnapshot snapshot, Point start_target, double start_t_delay,double start_shoot_vel, unsigned int max_func_evals)
		{
		double alpha = 0.5;

		unsigned int num_params = 4;
		double tiny_step = 0.0000000001;
		std::vector<double> current_params(num_params,0);
		std::vector<double> test_params(num_params,0);
		std::vector<double> weights(num_params,0);

		weights[0] = 1;
		weights[1] = 1;
		weights[2] = 2;
		weights[3] = 6;

		current_params[0] = start_target.x;
		current_params[1] = start_target.y;
		current_params[2] = start_t_delay;
		current_params[3] = start_shoot_vel;

		std::vector<double> grad;
		double norm_grad;
		double norm_grad_times_weights;
		double test_func_val;

		//evaluate current quality
		double current_func_val = ratePass(snapshot,Point(current_params[0],current_params[1]),current_params[2],current_params[3]);

		unsigned int func_evals = 1;

		while(func_evals <= (max_func_evals - num_params -1)){

			//find derivative of all the params
			grad = approximateGradient( snapshot ,current_params,  tiny_step, current_func_val, num_params, weights);
			func_evals = func_evals + num_params;

			
			//step the test params
			test_params = current_params;


			norm_grad = 0;
			for(unsigned int i = 0; i < num_params; i++){
				norm_grad += grad[i]*grad[i];
			}
			norm_grad = std::pow(norm_grad, 0.5);
			for(unsigned int i = 0; i < num_params; i++){
				//step: new = old + learningConst*weight*grad of this param/length of grad of all params
				 test_params[i] = test_params[i] + alpha*weights[i]*grad[i]/norm_grad;
			}

		
			//calculate the test func val
			test_func_val = ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3]);

			//if difference is large enough, increase the learning constant
			if ((test_func_val - current_func_val) > 0){
						alpha = 1.3*alpha;
						//std::cout << "successful point: " << test_func_val << std::endl;
			}

			else{
				while ((test_func_val - current_func_val) < 0 && func_evals <= max_func_evals){
					alpha = 0.5*alpha;
					//std::cout << "failed point: " << test_func_val << "   alpha: " << alpha << "   norm grad: " << norm_grad << std::endl;
					test_params = current_params;
					for(unsigned int i = 0; i < num_params; i++){
						test_params[i] = test_params[i] + alpha*weights[i]*grad[i]/norm_grad;
					}
					test_func_val = ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3]);
					func_evals ++;
				}
			}

			//if the algorithm helped, keep the new values
			if(test_func_val>current_func_val){
				current_params = test_params;
				current_func_val = test_func_val;
			}

		}

	std::vector<double> return_vals(2*num_params+2,0);
	return_vals[0] = current_func_val;
	for(unsigned int i = 0; i < num_params; i++){
		return_vals[i+1] = current_params[i];
	}
	return_vals[num_params +1] = alpha;

	for(unsigned int i = 0; i < grad.size(); i++){
			return_vals[i+6] = grad[i];
	}

	//quality,param0,...paramN,alpha
	return return_vals;

	}



std::vector<double> testOptimizePass(PassInfo::worldSnapshot snapshot, Point start_target, double start_t_delay,double start_shoot_vel, unsigned int max_func_evals){
		
		double alpha = 0.5;
		double beta = 0.001;
		unsigned int num_params = 4;
		double tiny_step = 0.0000000001;
		std::vector<double> current_params(num_params,0);
		std::vector<double> test_params(num_params,0);
		std::vector<double> weights(num_params,0);

		weights[0] = 1;
		weights[1] = 1;
		weights[2] = 2;
		weights[3] = 6;

		current_params[0] = start_target.x;
		current_params[1] = start_target.y;
		current_params[2] = start_t_delay;
		current_params[3] = start_shoot_vel;


		std::vector<double> grad;
		double norm_grad;
		double norm_grad_times_weights;
		double test_func_val;

		//evaluate current quality
		double current_func_val = ratePass(snapshot,Point(current_params[0],current_params[1]),current_params[2],current_params[3]);
		unsigned int func_evals = 1;


		while(func_evals <= (max_func_evals - num_params -1)){

			//find derivative of all the params
			grad = approximateGradient( snapshot ,current_params,  tiny_step, current_func_val, num_params, weights);
			func_evals = func_evals + num_params;

			
			//step the test params
			test_params = current_params;

			if(current_func_val > 0.03){

				for(unsigned int i = 0; i < num_params; i++){
					//step: new = old + learningConst*weight*grad of this param/length of grad of all params
					 test_params[i] = test_params[i] + beta*weights[i]*grad[i];
				}
				
				test_func_val = ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3]);
				func_evals ++;

					norm_grad_times_weights = 0;
				for(unsigned int i = 0; i < num_params; i++){
					norm_grad_times_weights += grad[i]*grad[i]*weights[i]*weights[i];
				}
				norm_grad_times_weights = std::pow(norm_grad_times_weights, 0.5);

				//std::cout << std::endl << "Current: " << current_func_val << "   test: " << test_func_val << "   Beta: " << beta << "  Norm grad*weights:  " << norm_grad_times_weights << std::endl;
					
				while(test_func_val < current_func_val && func_evals < max_func_evals){
					beta = beta*0.8;
					test_func_val = ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3]);
					func_evals++;
				}
				if(test_func_val > current_func_val){
					
					current_func_val = test_func_val;
					current_params = test_params;
					beta = beta*1.25;
				}
				
			}
			else{
				//length of the gradient vector
				norm_grad = 0;
				for(unsigned int i = 0; i < num_params; i++){
					norm_grad += grad[i]*grad[i];
				}
				norm_grad = std::pow(norm_grad, 0.5);
				for(unsigned int i = 0; i < num_params; i++){
				//step: new = old + learningConst*weight*grad of this param/length of grad of all params
				 test_params[i] = test_params[i] + alpha*weights[i]*grad[i]/norm_grad;
				}

				//weighted gradient lengths
				norm_grad_times_weights = 0;
				for(unsigned int i = 0; i < num_params; i++){
					norm_grad_times_weights += grad[i]*grad[i]*weights[i]*weights[i];
				}
				norm_grad_times_weights = std::pow(norm_grad_times_weights, 0.5);

				//calculate the test func val
				test_func_val = ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3]);

				//if difference is large enough, increase the learning constant
				if ((test_func_val - current_func_val) > 0.5*norm_grad*alpha){
							alpha = 1.3*alpha;
							
				}

				else{
					while ((test_func_val - current_func_val) < 0.5*norm_grad*alpha && func_evals <= max_func_evals){
						alpha = 0.5*alpha;
						
						test_params = current_params;
						for(unsigned int i = 0; i < num_params; i++){
							test_params[i] = test_params[i] + alpha*weights[i]*grad[i]/norm_grad;
						}
						test_func_val = ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3]);
						func_evals ++;
					}
				}
				//if the value improved keep it
				if(test_func_val>current_func_val){
					current_params = test_params;
					current_func_val = test_func_val;
				}
			}


			



		}


	std::vector<double> return_vals(2*num_params+2,0);
	return_vals[0] = current_func_val;
	for(unsigned int i = 0; i < num_params; i++){
		return_vals[i+1] = current_params[i];
	}
	return_vals[num_params +1] = alpha;

	for(unsigned int i = 0; i < grad.size(); i++){
			return_vals[i+6] = grad[i];
	}

	//quality,param0,...paramN,alpha
	return return_vals;

	}







	std::vector<double> approximateGradient(PassInfo::worldSnapshot snapshot, std::vector<double> params, double step_size, double current_func_val, int num_params, std::vector<double> weights )

		{

		//numerical derivative on all the params

		std::vector<double> gradient(num_params,0);
		for(int i = 0; i < num_params; i++) {
			std::vector<double>  test_params = params;
			test_params[i] += step_size*weights[i];
			gradient[i] = (ratePass(snapshot,Point(test_params[0],test_params[1]),test_params[2],test_params[3])  - current_func_val)/step_size;

		}

		return gradient;
	}
}
}
}
}
