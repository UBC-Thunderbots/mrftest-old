#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
using namespace AI::HL::STP::Action;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				DoubleParam block_threshold(u8"block threshold distance in terms of robot radius", u8"STP/Action/block", 3.0, 2.0, 8.0);
				DoubleParam block_angle(u8"baller projects a cone of this angle, blocker will avoid this cone (degrees)", u8"STP/Action/block", 5.0, 0, 90);
			}
		}
	}
}

void AI::HL::STP::Action::block_goal(World world, Player player, Robot robot) {
	Point dirToGoal = (world.field().friendly_goal() - robot.position()).norm();
	Point target = robot.position() + (block_threshold * Robot::MAX_RADIUS * dirToGoal);

	// #warning CHECK ??
	/*
	   // don't block from ball if we are blocking our baller!!
	   Player baller = Evaluation::calc_friendly_baller(world);
	   if (baller.is() && Evaluation::player_within_angle_thresh(baller, target, 2 * degrees2radians(block_angle))) {
	    Point target = robot.position() + (2 * block_threshold * Robot::MAX_RADIUS * dirToGoal);
	   }
	 */
	move(world, player, target);
}

void AI::HL::STP::Action::block_ball(World world, Player player, Robot robot) {
	Point dirToBall = (world.ball().position() - robot.position()).norm();
	Point target = robot.position() + (block_threshold * Robot::MAX_RADIUS * dirToBall);

	// #warning CHECK ??
	/*
	   // don't block from ball if we are blocking our baller!!
	   Player baller = Evaluation::calc_friendly_baller(world);
	   if (baller.is() && Evaluation::player_within_angle_thresh(baller, target, 2 * degrees2radians(block_angle))){
	    target = robot.position() + (2 * block_threshold * Robot::MAX_RADIUS * dirToBall);
	   }
	 */
	move(world, player, target);
}

