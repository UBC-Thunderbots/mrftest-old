#include "ai/tactic/chase.h"
#include "ai/navigator/testnavigator.h"

chase::chase(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), the_navigator(new testnavigator(player,field)) {
}

void chase::update() {
	point p = the_ball->position();
	the_player->move(p, the_player->orientation());
	the_navigator->update();
}
