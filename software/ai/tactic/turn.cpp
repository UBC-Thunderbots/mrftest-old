#include "ai/tactic/turn.h"

turn::turn(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) {
}

void turn::set_direction(const point& dir) {
	the_direction = dir;
}

void turn::update()
{
	// calculate orientation based on the target
	point target = the_direction - the_player->position();

	const point UP(1,0);
	UP.rotate(the_player->orientation());
	
	double theta = acos(UP.dot(target) / target.len());

	the_player->move(the_player->position(), the_player->orientation()+theta);
}
