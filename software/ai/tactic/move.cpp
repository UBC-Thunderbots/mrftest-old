#include "ai/navigator/testnavigator.h"
#include "ai/tactic/move.h"

move::move(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , the_navigator(new testnavigator(player,field,ball,team)) {
}

void move::set_position(const point& p)
{
	target_position = p;
}

void move::tick()
{
	// dribble if we have the ball
	the_player->dribble(the_player->has_ball()? 1:0);

	the_navigator->set_point(target_position);
	the_navigator->tick();
}
