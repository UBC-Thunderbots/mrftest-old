#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/world.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "ai/hl/world.h"
#include "ai/hl/stp/action/pivot.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;

void AI::HL::STP::Action::pivot(caller_t& ca, World world, Player player, Point target, Angle swing, double radius) {
	// set the destination point to be just behind the ball in the correct direction at the offset distance
	Point dest = -(target - world.ball().position()).norm() * radius + world.ball().position();
	//player.prio(AI::Flags::MovePrio::HIGH);

	Primitive prim = Primitives::Pivot(player, dest, swing, (world.ball().position() - dest).orientation());
	waitForPivot(ca, dest, player, 0.02);
}

