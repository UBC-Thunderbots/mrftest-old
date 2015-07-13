/* 
*  ratepass.cpp
* 
*   Created on: 2015-03-07
*       Author: cheng
*/

#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/gradient_approach/ratepass.h"
#include "ai/hl/stp/evaluation/staticPositionQuality.h"
#include "ai/hl/stp/evaluation/enemyRisk.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/friendlyCapability.h"
#include <iostream>

using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace AI {
	namespace HL {
		namespace STP {
			namespace GradientApproach {
				double ratePass(PassInfo::worldSnapshot snapshot, Point target,  double time_delay, double ball_velocity){
					if(snapshot.passee_positions.size() > 0){
						return 0;
					}
					double pass_quality = Evaluation::getStaticPositionQuality(snapshot, target);
					if(snapshot.passee_positions.size() > 0){
						pass_quality = pass_quality * (Evaluation::getFriendlyCapability(snapshot, target, time_delay, ball_velocity));
					}
			
					if(snapshot.enemy_positions.size() > 0){
						pass_quality = pass_quality * (1-Evaluation::getRatePassEnemyRisk(snapshot, target, time_delay, ball_velocity));
					}

					double shoot_score = Evaluation::get_passee_shoot_score(snapshot, target);
					pass_quality = pass_quality * (0.5 + 0.5*shoot_score );


					double dist = (snapshot.passer_position - target).len();
					pass_quality = pass_quality/(1+std::exp(3 * ( 1- dist))); 
					pass_quality = pass_quality/(1+std::exp(200 * ( - time_delay + 0.3)));
					pass_quality = pass_quality/(1+std::exp(200 * (2.5 - ball_velocity))); 
					pass_quality = pass_quality/(1+std::exp(200 * (-6 +  ball_velocity ))); 



					return pass_quality;

				}
			}
		}
	}
}
