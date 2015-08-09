#include "ai/flags.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/shoot.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam repel_dist(u8"Distance the defender should repel the ball in robot radius", u8"AI/HL/STP/Action/defend", 3.0, 2.0, 6.0);
	BoolParam shoot_not_repel(u8"Whether the defender shoot_goal (otherwise repels)", u8"AI/HL/STP/Action/defend", true);
}

void AI::HL::STP::Action::defender_move(World world, Player player, const Point dest) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	// if the ball is within repel distance do something
	if ((world.ball().position() - player.position()).len() < repel_dist * Robot::MAX_RADIUS) {
		// check to see if ball is too close to friendly goal
		if ((world.ball().position() - world.field().friendly_goal()).len() < repel_dist * 3) {
			// chip to the center of the field
			chip_target(world, player, Point(0,0));
		} else {
			// if we are far away from our own goal, if AI needs to be agressive, shoot goal
			if (shoot_not_repel) {
				shoot_goal(world, player);
			// otherwise AI should do defensive stuff
			} else {
				corner_repel(world, player);
			}
		}
		return;
	}

	// if we are not within repel distance, move closer
	player.mp_move(dest,(world.ball().position() - player.position()).orientation());

	player.type(AI::Flags::MoveType::NORMAL);
}

