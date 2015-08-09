#include <algorithm>
#include <cmath>

#include "ai/flags.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
}

bool AI::HL::STP::Action::chip_target(World world, Player player,
	const Point target)
{
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	return goalie_chip_target(world, player, target);
}

bool AI::HL::STP::Action::goalie_chip_target(World world, Player player,
	const Point target)
{
	// grab the ball in case we don't have it or lost it
	//intercept_pivot(world, player, target);

#warning "Does pivot work properly?"
/**
*	Due to navigator navigating at an offset of ~10 degrees, ball is never
*	hitting directly on chipper. This results in the player continously ramming
*	the ball, which is bad for kickoffs. The following code will be uncommented
*	once pivotting properly works.
*/

	// angle is right but chicker not ready, ram the ball and get closer to target
	if (!player.chicker_ready()) {
//		LOG_INFO(u8"chicker not ready, in Action::chip_target");
//		// angle is right but chicker not ready, ram the ball and get closer to
//		// target, only use in normal play
//		if (world.playtype() == AI::Common::PlayType::PLAY) {
//			const Point diff = world.ball().position() - player.position();
//			ram(world, player, target, diff * FAST);
//		}
		return false;
	}

	//default chip distance for any legacy usage of this action
	player.mp_shoot(world.ball().position(),
		(target - world.ball().position()).orientation(), true,
		(target - player.position()).len());
	LOG_INFO(u8"autochip, Action::chip_target");

	return true;
}

bool AI::HL::STP::Action::chip_at_location(Player player,
	const Point location_to_chip_at, double chip_distance, Angle chip_orientation)
{
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_shoot(location_to_chip_at, chip_orientation, true, chip_distance);

	return true;
}


