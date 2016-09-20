#include "ai/hl/stp/action/legacy_action.h"
#include <algorithm>
#include <cmath>

#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_cshoot.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/stp/action/legacy_ram.h"
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
		// angle is right but chicker not ready, ram the ball and get closer to
		// target, only use in normal play
		if (world.playtype() == AI::Common::PlayType::PLAY) {
			ram(world, player, target);
		}
		return false;
	}

	player.mp_shoot(player.position(), player.orientation(), false, velocity);
	return true;
}
