#include "ai/hl/stp/action/legacy_action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_intercept.h"

BoolParam LEGACY_INTERCEPT_MP_MOVE(u8"(Legacy) Use mp_move or MP_intercept", u8"AI/HL/STP/Action/Shoot", true);
void AI::HL::STP::Action::intercept(AI::HL::STP::Player player, const Point target) {
	//slow down the dribbler to make it easier to catch the ball
	//mp_catch(world.ball().position());
	if (LEGACY_INTERCEPT_MP_MOVE)
	{
		player.mp_move(target, (target - player.position()).orientation());
	}
	else
	{
		player.mp_dribble(target, (target - player.position()).orientation());
	}
}



