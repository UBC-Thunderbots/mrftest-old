#include "ai/tactic/kick.h"

kick::kick(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) {
}

void kick::set_target(const point& p) {
	the_target = p;
}

void kick::update() {
	// calculate orientation based on the target
	point target = the_target - the_player->position();

	point orient(1,0);
	orient.rotate(the_player->orientation());
	
	double theta = acos(orient.dot(target) / target.len());

	// so far, we assume the passing player stays still while turning
	the_player->move(the_player->position(), the_player->orientation()+theta);

	// assume maximum strength for now...
	the_player->kick(1);
}

