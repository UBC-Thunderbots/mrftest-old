#include "ai/tactic/pass.h"

pass::pass(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) {
}

void pass::update() {
//	point target = calculate_target();

	// calculate orientation based on the target
	point target = receiver->position() - the_player->position();

	point orient(1,0);
	orient.rotate(the_player->orientation());
	
	double theta = acos(orient.dot(target) / target.len());

	// so far, we assume the passing player stays still while turning
	the_player->move(the_player->position(), the_player->orientation()+theta);

	// assume maximum strength for now...
	the_player->kick(1);
}

point pass::calculate_target() 
{
	switch (mode) {
		case STATIONARY:
		case FULL_SPEED:
		case OPEN:
		default:
		break;
	}
	return point(0,0);
}


