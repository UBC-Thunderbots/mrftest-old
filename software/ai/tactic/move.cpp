#include "ai/tactic/move.h"
#include "ai/navigator/testnavigator.h"

move::move(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , the_navigator(new testnavigator(player,field)) {
	target_orientation = player->orientation();
}

void move::set_position(const point& p)
{
	target_position = p;
}

void move::set_orientation(const double& orientation)
{
	target_orientation = orientation;
}

void move::update()
{
	the_player->move(target_position, target_orientation);
	the_navigator->update();
}
