#include "ai/tactic/move.h"

move::move(player::ptr player, world::ptr world) : the_player(player), navi(player, world), position_initialized(false), orientation_initialized(false) {
	avoid_ball = false;
}

void move::set_avoid_ball(bool avoid){
	avoid_ball = avoid;
}

void move::tick() {
#warning refactor
	navi.set_robot_avoids_ball(avoid_ball);
#warning logic error, if we use speed sensing then has_ball is always false when not dribbling
	the_player->dribble(the_player->has_ball()? 1:0);

	if (position_initialized) navi.set_position(target_position);
	if (orientation_initialized) navi.set_orientation(target_orientation);
	navi.tick();

	position_initialized = false;
	orientation_initialized = false;
}

