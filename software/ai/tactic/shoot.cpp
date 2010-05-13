#include "ai/tactic/shoot.h"
#include "geom/angle.h"

shoot::shoot(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), chase_tactic(new chase(ball, field, team, player)), kick_tactic(new kick(ball, field, team, player)) {
}

void shoot::tick()
{
	if (!the_player->has_ball()) 
	{
		bool has_ball = false;
		for (unsigned int i = 0; i < the_team->size(); ++i) 
		{
			if (the_team->get_player(i)->has_ball()) 
			{
				has_ball = true;
				break;
			}
		}
		// chase if our team does not have the ball
		if (!has_ball)
		{
			chase_tactic->tick();
		}
	} 
	else
	{
		point shooting_points[SAMPLE_POINTS];

		// allow some space for the ball to go in from the post
		const double EDGE_SPACE = 0.1;

		double goal_width = (the_field->goal_width() - EDGE_SPACE) * 2;
		double delta = goal_width / SAMPLE_POINTS;

		for (unsigned int i = 0; i < SAMPLE_POINTS; ++i)
		{
			point p(the_field->length(), -the_field->goal_width() + EDGE_SPACE + i * delta);
			shooting_points[i] = p;
		}

		int best_point = -1;
		double best_score = -1;

		team::ptr opponent_team = the_team->other();

		double proximity, score, dist;
		for (unsigned int i = 0; i < SAMPLE_POINTS; ++i)
		{		
			point projection = shooting_points[i] - the_player->position();
			score = 0;
			for (unsigned int i = 0; i < opponent_team->size(); ++i)
			{
				// TODO: take into account of velocity?
				point other = opponent_team->get_robot(i)->position() - the_player->position();

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
				
					if (best_point == -1 || score < best_score)
					{
						best_point = i;
						best_score = score;
					}
				}
			}
		}		

		// if all the points are equally bad (opponent robot in all projections), just use the first point and pray
		if (best_point == -1)
		{
			best_point = 0;
		}

		// shoot
		kick_tactic->set_target(shooting_points[best_point]);
		kick_tactic->tick();	
	}
}
