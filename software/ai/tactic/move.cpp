//#include "ai/navigator/collisionnavigator.h"
#include "ai/navigator/robot_navigator.h"
#include "ai/navigator/testnavigator.h"
#include "ai/tactic/move.h"

move::move(player::ptr player, world::ptr world) : the_player(player), avoid_ball(false), the_navigator(player, world) {
}

void move::set_position(const point& p)
{
	target_position = p;
}

void move::set_avoid_ball(bool avoid){
avoid_ball = avoid;

}

void move::tick()
{

	the_navigator.set_robot_avoids_ball(avoid_ball);
#warning logic error, if we use speed sensing then has_ball is always false when not dribbling
	the_player->dribble(the_player->has_ball()? 1:0);

	the_navigator.set_point(target_position);
	the_navigator.tick();
}
