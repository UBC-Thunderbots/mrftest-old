#include "ai/tactic/shoot.h"
#include "geom/angle.h"
#include "ai/util.h"
#include <vector>

shoot::shoot(player::ptr player, world::ptr world) : tactic(player), the_world(world), chase_tactic(player, world), kick_tactic(player, world) {
}

void shoot::tick()
{
	const friendly_team &the_team(the_world->friendly);

#warning has ball here
	if (!the_player->sense_ball()) 
	{
		bool sense_ball = false;
		for (unsigned int i = 0; i < the_team.size(); ++i) 
		{
			if (the_team.get_player(i)->sense_ball()) 
			{
				sense_ball = true;
				break;
			}
		}
		// chase if our team does not have the ball
		if (!sense_ball)
		{
			chase_tactic.set_flags(flags);
			chase_tactic.tick();
		}
	} 
	else
	{
		std::vector<point> candidates = ai_util::calc_candidates(the_world);
		int best_point = ai_util::calc_best_shot(the_player, the_world);
		// if all the points are equally bad (opponent robot in all projections), just use the first point and pray
		if (best_point == -1)
		{
			best_point = 0;
		}

		// shoot
		kick_tactic.set_target(candidates[best_point]);
		kick_tactic.tick();	
	}
}

