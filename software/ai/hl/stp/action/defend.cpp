#include "defend.h"

#include "ai/hl/stp/action/action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/predicates.h"
#include "chip.h"
#include "repel.h"
#include "util/param.h"
#include "util/dprint.h"

using namespace AI::HL::STP;
namespace Action = AI::HL::STP::Action;

namespace {
	DoubleParam repel_dist(u8"Distance the defender should repel the ball in robot radius", u8"AI/HL/STP/Action/defend", 3.0, 2.0, 6.0);
	BoolParam shoot_not_repel(u8"Whether the defender shoot_goal (otherwise repels)", u8"AI/HL/STP/Action/defend", true);
}

void AI::HL::STP::Action::defender_move(caller_t& ca, World world, Player player, Point dest, bool active_baller) {
	// if the ball is within repel distance do something
	if ((world.ball().position() - player.position()).len() < repel_dist * Robot::MAX_RADIUS && !active_baller) {
		// check to see if ball is too close to friendly goal
		if ((world.ball().position() - world.field().friendly_goal()).len() < repel_dist * 3) {
			// chip to the center of the field
			chip_target(ca, world, player, Point(0,0));
		} else {
			// if we are far away from our own goal, if AI needs to be agressive, shoot goal
			if (shoot_not_repel) {
				shoot_goal(ca, world, player);
			// otherwise AI should do defensive stuff
			} else {
				corner_repel(ca, world, player);
			}
		}
		return;
	}

	Point new_dest = dest;
	if ((dest - world.ball().position()).lensq() < 0.3 * 0.3 && active_baller) {
		Point norm = (dest - world.ball().position()).lensq() > 1e-9 ? (dest - world.ball().position()).norm() : Point(0, 1);
		// don't get too close as to interfere with baller
		new_dest = world.ball().position() + norm * 0.3;
	}

	// if we are not within repel distance, move closer
	Action::move(ca, world, player, new_dest, (world.ball().position() - player.position()).orientation());
}

