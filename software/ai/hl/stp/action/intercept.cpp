#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"

BoolParam INTERCEPT_MP_MOVE(u8"Use mp_move or MP_intercept", u8"AI/HL/STP/Action/Shoot", true);
void AI::HL::STP::Action::intercept(AI::HL::STP::Player player, const Point target) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.type(AI::Flags::MoveType::INTERCEPT);
	//slow down the dribbler to make it easier to catch the ball
	//mp_catch(world.ball().position());
	if(INTERCEPT_MP_MOVE)
	{
		player.mp_move(target, (target - player.position()).orientation());
	}
	else
	{
		player.mp_dribble(target, (target - player.position()).orientation());
	}
}



