//Takes enemy positions and velocities, passee, passer, pass destination, kick speed, and delay time as input
//Evaluates the risk due to pass interception and proximity of enemy robots to destination position
//Returns a value between 0 and 1: where a higher number represents greater risk
#include "ai/hl/world.h"
#include "geom/point.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/enemyRisk.h"
#include <math.h>
#include <vector>
#include <stdio.h>
#include <iostream>
 

using namespace AI::HL::STP;
using namespace AI::HL::W;
using namespace AI::HL::STP::GradientApproach;

	double ENEMY_PROXIMITY_IMPORTANCE = 0.5;

	





	double AI::HL::STP::Evaluation::getRatePassEnemyRisk(PassInfo::worldSnapshot snapshot, Point destination, double delay_time, double kickSpeed){

		double quality = 1; //start assuming there is no risk

		//increase the risk at a given position based on the distance from all enemy robots
		for (Point each_enemy_position : snapshot.enemy_positions) {
			double distance = (each_enemy_position - destination).len();
			quality = quality*(1-ENEMY_PROXIMITY_IMPORTANCE*std::exp(-distance*distance));

		}

		//include the danger of the pass being intercepted
		quality = quality*(1 - dangerInterception(snapshot,destination, delay_time, kickSpeed));

		//convert the pass quality to a risk
		double risk = 1 -quality;

		return risk;



	}

	double AI::HL::STP::Evaluation::dangerInterception(PassInfo::worldSnapshot snapshot, Point destination, double delay_time, double kickSpeed){

		double R_RADIUS = Robot::MAX_RADIUS;

		//ENEMY_A_MAX and ENEMY_V_MAX should be pulled from the logs
		double ENEMY_A_MAX = 3;
		double ENEMY_V_MAX = 2;

		//Estimate of the time it takes the enemy to respond to our actions, should be observed
		double ENEMY_T_REACT = 0.35;

		//Constants used to tune the sharpness of quality functions
		double DIST_UNCERTAINTY = 0.4;
		double TIME_UNCERTAINTY = 3;
		double W_q = 9;

		std::size_t num_enemies = snapshot.enemy_positions.size();
		Point passer_pos = snapshot.passer_position;
		//assumes ball maintains constant velocity- should be improved later
		double t_arrive = (passer_pos - destination).len()/kickSpeed + delay_time;
		double max_danger = 0;
		double future_time;
		//Future time is used to predict enemy positions when the ball is kicked.
		//Enemies are assumed to move at constant velocity then stop in their position
		//at 'future_time'
		if (delay_time > 0.45){future_time = 0.45;}
		else{future_time = delay_time;}


		//This has 1 more element than enemy_team because a worst case prediction for the closest enemy robot is added.
		//The closest enemy robot is assumed to move as fast as possible (without crashing) towards the passer
		std::vector<Point> projected_enemy_positions(num_enemies +1);
		double shortest_distance = 1000;//start with a very large number
		Point projected_closest_enemy_position;
		double current_distance;


		for (std::size_t i = 0; i < snapshot.enemy_positions.size(); ++i){
			projected_enemy_positions.at(i) = snapshot.enemy_positions.at(i)
													+ snapshot.enemy_velocities.at(i) * future_time;
			current_distance = (snapshot.passer_position - projected_enemy_positions.at(i)).len();
			if (current_distance < shortest_distance){
				//calculates closest enemy robot at future_time
				projected_closest_enemy_position = projected_enemy_positions.at(i);
				shortest_distance = current_distance;
			}
		}



		double distance_travelled;
		//Finds how far the closest enemy could travel (calculated based on zero initial velocty)
		//The initial velocity is then multiplied by future_time and added to the result to try to
		//account for the zero initial velocity assumption
		if (delay_time > 2*ENEMY_V_MAX/ENEMY_A_MAX){
				//distance that can be travelled by accelerating to max speed, going at max speed, then
				//decelerating to zero velocity
				//oversimplified- they will actually start slowing down at the end not the middle
				distance_travelled = ENEMY_V_MAX*ENEMY_V_MAX/ENEMY_A_MAX + (delay_time - 2*ENEMY_V_MAX/ENEMY_A_MAX)*ENEMY_V_MAX;
		}
			else{
				//distance by accelerating half the time, then decelerating
				distance_travelled = ENEMY_A_MAX/4*delay_time*delay_time;
		}

		//Additional virtual enemy to be avoided- works on assumption that closest enemy to ball moves towards ball
		if (shortest_distance > distance_travelled){
				projected_enemy_positions.at(num_enemies) = projected_closest_enemy_position*(1-distance_travelled/shortest_distance) + passer_pos*(distance_travelled/shortest_distance);
		}
			else{
				projected_enemy_positions.at(num_enemies) = passer_pos ;
		}



		double q;
		double r;
		double l2;
		double dist_intercept;
		double t_intercept;
		double danger;

		for(std::size_t i=0; i<=num_enemies ;i++){

			l2 = (destination - passer_pos).len();
			l2 = l2*l2;
			r = ((projected_enemy_positions.at(i) - passer_pos).dot(destination - passer_pos))/l2;

			//find t_intercept
			if (r < 0)  {
				dist_intercept = (projected_enemy_positions.at(i) - passer_pos).len();
				t_intercept = delay_time;
			}
			else if (r > 1){
				dist_intercept = (projected_enemy_positions.at(i) - destination).len();
				t_intercept = t_arrive;
			}
			else{
				Point projected_vec = passer_pos + r*(destination - passer_pos);
				dist_intercept = (projected_enemy_positions.at(i) - projected_vec).len();
				t_intercept = delay_time + r*(t_arrive - delay_time);
			}

			//badly named, will change later
			double t_move = (t_intercept - delay_time - ENEMY_T_REACT);


			//find q
			if (t_move > 2*ENEMY_V_MAX/ENEMY_A_MAX){
			  q = dist_intercept -R_RADIUS-DIST_UNCERTAINTY -ENEMY_V_MAX*ENEMY_V_MAX/ ENEMY_A_MAX - (t_move - 2*ENEMY_V_MAX/ENEMY_A_MAX)*ENEMY_V_MAX;
			}
			else if (t_move > 0 ){
			  q = dist_intercept -R_RADIUS- DIST_UNCERTAINTY- ENEMY_A_MAX/4*t_move*t_move ;
			}
			else {
				q = dist_intercept - R_RADIUS - DIST_UNCERTAINTY;
			}

			danger = 1/(1+std::exp(W_q*q/((TIME_UNCERTAINTY*delay_time+1)*(t_intercept - delay_time))));

			if (danger > max_danger){
				max_danger = danger;
			}


		}

		return max_danger;
	}
