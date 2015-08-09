#include <algorithm>
#include <cmath>

#include "ai/flags.h"
#include "ai/hl/stp/action/cshoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/rect.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

bool AI::HL::STP::Action::cshoot_target(World world, Player player,
	const Point target, double velocity)
{
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	Action::move(world, player, world.ball().position()+
		0.04*(world.ball().position() - target).norm());

	// if (shoot_data.can_shoot) {
	if (!Evaluation::player_within_angle_thresh(player, target,
		passer_angle_threshold))
	{
		return false;
	}

	// angle is right but chicker not ready, ram the ball and get closer to target
	if (!player.chicker_ready()) {
		LOG_INFO(u8"chicker not ready");
		// angle is right but chicker not ready, ram the ball and get closer to
		// target, only use in normal play
		if (world.playtype() == AI::Common::PlayType::PLAY) {
			ram(world, player, target);
		}
		return false;
	}

	LOG_INFO(u8"autokick");
	player.mp_shoot(player.position(), player.orientation(), false, velocity);
	return true;
}
