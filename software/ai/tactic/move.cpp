//#include "ai/navigator/collisionnavigator.h"
#include "ai/navigator/robot_navigator.h"
#include "ai/navigator/testnavigator.h"
#include "ai/tactic/move.h"

move::move(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , avoid_ball(false), the_navigator(new robot_navigator(player,field,ball,team)) {
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

	robot_navigator::ptr rbt = robot_navigator::ptr::cast_dynamic(the_navigator);
	rbt->set_robot_avoids_ball(avoid_ball);
	the_player->dribble(the_player->has_ball()? 1:0);

	the_navigator->set_point(target_position);
	the_navigator->tick();
}
