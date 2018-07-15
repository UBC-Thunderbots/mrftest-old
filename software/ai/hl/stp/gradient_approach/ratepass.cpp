/*
*  ratepass.cpp
*
*   Created on: 2015-03-07
*       Author: cheng
*/

#include "ai/hl/stp/gradient_approach/ratepass.h"
#include <iostream>
#include "ai/hl/stp/evaluation/enemy_risk.h"
#include "ai/hl/stp/evaluation/friendly_capability.h"
#include "ai/hl/stp/evaluation/move.h"
#include "ai/hl/stp/evaluation/static_position_quality.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"

using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace AI {
	namespace HL {
		namespace STP {
			namespace GradientApproach {
				double ratePass(PassInfo::worldSnapshot snapshot, Point target,  double time_delay, double ball_velocity){
					
					double pass_quality = Evaluation::getStaticPositionQuality(snapshot, target);

					if(snapshot.passee_positions.size() > 0){
						// prefer locations we can get to
						pass_quality = pass_quality * (Evaluation::getFriendlyCapability(snapshot, target, time_delay, ball_velocity));
					}else{
                        pass_quality = 0; //no one to pass to
                    }

			
					if(snapshot.enemy_positions.size() > 1){
						// avoid areas enemy robots can get to
						pass_quality = pass_quality * (1-Evaluation::getRatePassEnemyRisk(snapshot, target, time_delay, ball_velocity));
						double enemy_dist = Evaluation::closestEnemyDist(snapshot);
						double time_factor_dist = time_delay;
						// weight based on closest enemy and delay time
						// todo: add flag for free kicks that doesnt care about closest enemy taking the ball from passer
						double time_factor = 1/(1+std::exp(3 * (time_factor_dist - enemy_dist ))); // prefer passes where time factor is less than enemy dist 
						pass_quality = pass_quality*(0.05 + 0.95*time_factor);
					}

					double shoot_score = Evaluation::get_passee_shoot_score(snapshot, target);
					pass_quality = pass_quality * (0.3 + 0.7*shoot_score ); // give some importance to shooting (but not all)

					double dist = (snapshot.passer_position - target).len();
					pass_quality = pass_quality/(1+std::exp(3 * ( 1- dist))); // prefer passes more than a metre away 
					pass_quality = pass_quality/(1+std::exp(200 * ( - time_delay + 0.3))); // prefer passes more than 0.3 seconds away
					pass_quality = pass_quality/(1+std::exp(200 * (2.0 - ball_velocity))); // strict requirement that ball vel > 1.5
					pass_quality = pass_quality/(1+std::exp(200 * (-4 +  ball_velocity )));  // strict requirement that ball vel < 4

					return std::max(0.0,pass_quality);
				}
			}
		}
	}
}
