#include "ai/flags.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
using namespace AI::HL::STP::Action;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				DoubleParam block_threshold(u8"block threshold distance in terms of robot radius", u8"AI/HL/STP/Action/block", 3.0, 2.0, 8.0);
				DoubleParam block_angle(u8"baller projects a cone of this angle, blocker will avoid this cone (degrees)", u8"AI/HL/STP/Action/block", 5.0, 0, 90);
			}
		}
	}
}

void AI::HL::STP::Action::block_goal(World world, Player player, Robot robot) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	Point dirToGoal = (world.field().friendly_goal() - robot.position()).norm();
	Point target = robot.position() + (block_threshold * Robot::MAX_RADIUS * dirToGoal);

	player.mp_move(target, (world.ball().position() - player.position()).orientation());
}

void AI::HL::STP::Action::block_ball(World world, Player player, Robot robot) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	Point dirToBall = (world.ball().position() - robot.position()).norm();
	Point target = robot.position() + (block_threshold * Robot::MAX_RADIUS * dirToBall);

	player.mp_move(target, (world.ball().position() - player.position()).orientation());
}
