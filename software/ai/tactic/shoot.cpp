#include "ai/tactic/shoot.h"

shoot::shoot(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), chase_tactic(new chase(ball, field, team, player)), kick_tactic(new kick(ball, field, team, player)) {
}

void shoot::tick()
{
	if (!the_player->has_ball()) 
	{
		bool has_ball = false;
		for (unsigned int i = 0; i < the_team->size(); ++i) 
			if (the_team->get_robot(i)->has_ball()) {
				has_ball = true;
				break;
			}
		// chase if our team does not have the ball
		if (!has_ball)
			chase_tactic->tick();
	} 
	else
	{
		// shoot
		point enemy_goal(the_field->length(), 0);

		// TODO: detect where other goalie is and shoot at opening		
		kick_tactic->set_target(enemy_goal);
		kick_tactic->tick();
		
	}
}
