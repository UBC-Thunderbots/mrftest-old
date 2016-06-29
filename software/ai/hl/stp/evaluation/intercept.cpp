/*
 * intercept.cpp
 *
 *  Created on: Dec 2, 2015
 *      Author: mathew
 */

#include "intercept.h"
#include "ai/hl/world.h"
#include "ai/hl/util.h"
#include "geom/point.h"
#include "geom/util.h"
#include "ai/hl/util.h"
#include "ai/common/objects/field.h"
#include <math.h>

using namespace AI::HL::W;
using namespace AI::HL::STP;
using namespace AI::HL::Util;


double AI::HL::STP::Evaluation::getBestIntercept(World world, Player interceptor, Point predicted_intercept_pos, double time) {
/* Notes:
 * -Consider time to reach point - less is better
 * -Consider how close enemy is - farther away is better
 * 		-if enemy can intercept, no point after that point should be considered - should be handled in tactic
 *  -Consider shooting and passing opportunities at that point - more is better
 *
 * Weights:
 * 	-time and enemy threat are most important
 * 	-potential passes and shots least important
 * 	-enemy threat should trump all others after certain threshold ie. if they can intercept
 *
 * 	Evaluation:
 * 		-use weights to assign a score to each point, and the point with the highest score is chosen
 *
 *
 *
 */

	double R_Radius = Robot::MAX_RADIUS;
	//double e = 2.71828182845904523536028747135266249775724709369995;

	double score = 0.0;
	double time_score = 1.0;
	double enemy_score = 1.0;
	double shots_score = 0.0;
	double threat_dist = 2.0;//If enemies are less than this distance away from the point, they are considered as a threat in the evaluation. Otherwise they are ignored

	//set up evaluation weights
	int time_weight = 500;
	int enemy_weight = 30;
	int shots_weight = 100;

	//Evaluate time
	time_score = -time_weight/100 * time * exp(time/1.5) + 100;

	//Potentially choose to only evaluate several nearby players
	EnemyTeam enemies = world.enemy_team();
	Robot enemy;
	Point e_pos;
	double e_dist;
	for(unsigned int i=0; i < enemies.size(); i++) {//++i?
		enemy = enemies[i];
		e_pos = enemy.position();
		e_dist = (predicted_intercept_pos - e_pos).len() - R_Radius;//dist from closest edge of robot

		enemy_score += pow(((threat_dist - e_dist) / (e_dist / (enemy_weight / 500))),3.0);

		if(enemy_score > 500) {
			enemy_score = 500;
		}
	}


	//Evaluate potential shots
	Angle shot_angle = AI::HL::Util::calc_best_shot(world, interceptor).second;
	Point gp_1 = world.field().enemy_goal_boundary().first;
	Point gp_2 = world.field().enemy_goal_boundary().second;
	Point dir_1 = gp_1 - predicted_intercept_pos;
	Point dir_2 = gp_2 - predicted_intercept_pos;
	Angle total_angle = Angle::acos(dir_1.dot(dir_2) / (dir_1.len() * dir_2.len()));//total angle between each goalpost
	double shots = shot_angle / total_angle;

	shots_score = shots_weight / 10 * shots;



	//Create final score and return
	score = time_score / enemy_score + shots_score;


	if(score < 0) {
		score = 0.0;
	}

	return score;

}




