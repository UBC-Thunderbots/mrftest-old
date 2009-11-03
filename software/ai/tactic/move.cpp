#include "ai/tactic/move.h"
#include "ai/navigator/testnavigator.h"

move::move(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , the_navigator(new testnavigator(player,field)) {
}

void move::set_position(const point& p)
{
	target_position = p;
}

void move::update()
{
	the_navigator->set_point(target_position);
	the_navigator->update();
}
