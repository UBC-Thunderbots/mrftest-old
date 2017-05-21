#include "chip.h"

#include "ai/hl/stp/action/action.h"
#include <algorithm>
#include <cmath>

#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
}

bool AI::HL::STP::Action::chip_target(caller_t& ca, World world, Player player,
	const Point target)
{
	return goalie_chip_target(ca, world, player, target);
}

bool AI::HL::STP::Action::goalie_chip_target(caller_t& ca, World world, Player player,
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
//		// angle is right but chicker not ready, ram the ball and get closer to
//		// target, only use in normal play
//		if (world.playtype() == AI::Common::PlayType::PLAY) {
//			const Point diff = world.ball().position() - player.position();
//			ram(world, player, target, diff * FAST);
//		}
		return false;
	}

	//default chip distance for any legacy usage of this action
	// chips in the direction the robot is facing
	Action::shoot_target(ca, world, player, Point::of_angle((target - world.ball().position()).orientation()).norm(10), (target - player.position()).len(), true);
	LOG_DEBUG(u8"autochip, Action::chip_target");

	return true;
}

bool AI::HL::STP::Action::chip_at_location(caller_t& ca, World world, Player player,
	const Point location_to_chip_at, double chip_distance, Angle chip_orientation)
{
	Action::shoot_target(ca, world, player, location_to_chip_at, chip_distance, true);
	return true;
}


