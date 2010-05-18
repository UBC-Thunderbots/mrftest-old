#include "ai/util.h"

namespace ai_util{
//
// Number of points to consider when shooting at the goal.
//
const unsigned int SHOOTING_SAMPLE_POINTS = 9;

const std::vector<point> calc_candidates(const world::ptr world){
	std::vector<point> candidates(SHOOTING_SAMPLE_POINTS);
	const field &the_field(world->field());

	// allow some space for the ball to go in from the post
	const double EDGE_SPACE = 0.1;

	double goal_width = (the_field.goal_width() - EDGE_SPACE) * 2;
	double delta = goal_width / SHOOTING_SAMPLE_POINTS;

	for (unsigned int i = 0; i < SHOOTING_SAMPLE_POINTS; ++i)
	{
		point p(the_field.length(), -the_field.goal_width() + EDGE_SPACE + i * delta);
		candidates[i] = p;
	}
	return candidates;
}

size_t calc_best_shot(player::ptr player, const world::ptr world){
	std::vector<point> candidates = calc_candidates(world);
	size_t best_point = candidates.size();
	double best_score = -1;

	const team &opponent_team(world->enemy);
	double proximity, score, dist;

	for (unsigned int i = 0; i < SHOOTING_SAMPLE_POINTS; ++i)
	{		
		point projection = candidates[i] - player->position();
		score = 0;
		for (unsigned int i = 0; i < opponent_team.size(); ++i)
		{
#warning TODO: take into account of velocity?
			point other = opponent_team.get_robot(i)->position() - player->position();
			proximity = (other).dot(projection.norm());
			// don't process the robot if it's behind the shooter
			if (proximity >= robot::MAX_RADIUS)
			{
				// calculate how close the opponent robot is to our robot in proportion to our projection, 0 if the opponent robot is
				// at our robot, 1 if the opponent robot is at the target.			
				// scale_factor = proximity / projection.len();
		
				dist = sqrt(other.lensq() - proximity * proximity);
		
				if (dist <= robot::MAX_RADIUS)
				{
					break;
				}	
				// use a 1/dist function to determine to score: the closer the opponent robot is to the projection, the higher the score
				score += 1.0 / dist;
			
				if (best_point == candidates.size() || score < best_score)
				{
					best_point = i;
					best_score = score;
				}
			}
		}
	}
	return best_point;
}

}
