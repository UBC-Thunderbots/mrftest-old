#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"

BoolParam INTERCEPT_MP_MOVE(u8"Use mp_move or MP_intercept", u8"AI/HL/STP/Action/Shoot", true);

void AI::HL::STP::Action::intercept(caller_t& ca, World, Player player, const Point target) {
	// mp_catch(world.ball().position());
	//
	if (INTERCEPT_MP_MOVE)
	{
		const Primitive& prim = Primitives::Move(player, target, (target - player.position()).orientation());
		Action::wait(ca, prim);
	}
	else
	{
		const Primitive& prim = Primitives::Dribble(player, target, (target - player.position()).orientation(), false);
		Action::wait(ca, prim);
	}
}
