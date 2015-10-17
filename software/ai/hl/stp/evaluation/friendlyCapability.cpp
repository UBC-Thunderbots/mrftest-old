#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/evaluation/friendlyCapability.h"
#include "ai/hl/world.h"
#include "ai/hl/util.h"
#include "geom/point.h"
#include "geom/angle.h"
#include <math.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdio.h>

using namespace AI::HL::W;
using namespace AI::HL::STP;
using namespace AI::HL::Util;
using namespace AI::HL::STP::GradientApproach;

	
	double AI::HL::STP::Evaluation::getFriendlyCapability(PassInfo::worldSnapshot snapshot, Point dest, double t_delay, double ball_vel) {
			//setup constants


			double A_MAX = 3;
			double V_MAX = 2;
			double SCALING_CONST = 4;
			double friendlyCapability = 1;
			double shortest_dist = 1000; //Start with a large number
			Robot best_passee;
			double current_dist;

			for(Point each_passee_position : snapshot.passee_positions){
				current_dist = (dest - each_passee_position).len();
				if(current_dist < shortest_dist){
					shortest_dist = current_dist;
				}
			}

			//total time is delay time + the time it takes the ball to reach its destination
			double total_time=t_delay+(snapshot.passer_position-dest).len()/ball_vel;
			//the distance the passee is from the destination
			double passee_dist = shortest_dist;
			double r=0;

			//can we get there in time?
			if (total_time > 2*V_MAX/A_MAX){
			    r = V_MAX*V_MAX/A_MAX + (total_time - 2*V_MAX/A_MAX)*V_MAX - passee_dist;
			}

			else{
			    r =  A_MAX/4*total_time*total_time - passee_dist;
			}

			friendlyCapability =  friendlyCapability/(1+std::exp(-SCALING_CONST * r));

			//Can the passer turn that fast
			
			double angle_dif = (dest - snapshot.passer_position).orientation().angle_diff(snapshot.passer_orientation).to_degrees();
			friendlyCapability = friendlyCapability/(1+std::exp(  0.2*(angle_dif - 10 - t_delay*360)));

			return friendlyCapability;

		}
