#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/world.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "ai/hl/world.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/evaluation/plan_util.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;
namespace Plan = AI::HL::STP::Evaluation::Plan;

namespace {
	DoubleParam DEFAULT_FINAL_VEL(u8"the default final velocity for points generated while pivoting", u8"AI/HL/STP/Action/pivot", 0.75, 0.0, 5.0);
	IntParam ROTATION(u8"Amount of rotation for each point (in degrees)", u8"AI/HL/STP/Action/pivot", 10, 0, 10);
}

void AI::HL::STP::Action::pivot(caller_t& ca, World world, Player player, Point target, Angle finalAngle, double radius) {
	// Old implementation. remove later
//	// set the destination point to be just behind the ball in the correct direction at the offset distance
//	Point dest = -(target - world.ball().position()).norm() * radius + world.ball().position();
//	//player.prio(AI::Flags::MovePrio::HIGH);
//
//	Primitive prim = Primitives::Pivot(player, dest, swing, (world.ball().position() - dest).orientation());
//	waitForPivot(ca, dest, player, 0.02);

	# warning RoboCup 2017 hack. Call moves around a point to simulate pivot. This should use the pivot primitive once it works

	Point dest = Point();
	double finalVel = DEFAULT_FINAL_VEL;
	if(std::fabs((player.position() - target).len() - radius) > 0.02) {
		dest = target + (player.position() - target).norm(radius);
	}else {
		// if the points generated will rotate clockwise around the target
		bool clockwise = is_clockwise((player.position() - target), Point::of_angle(finalAngle));
		if((player.position() - target).orientation().angle_diff(finalAngle) <= Angle::of_degrees(ROTATION)) {
			dest = target + Point::of_angle(finalAngle).norm(radius);
			finalVel = 0.0;
		}else {
			if(clockwise) {
				dest = (player.position() - target).norm(radius).rotate(Angle::of_degrees(-ROTATION));
			}else {
				dest = (player.position() - target).norm(radius).rotate(Angle::of_degrees(ROTATION));
			}
		}
	}

	player.display_path(std::vector<Point>{dest});
	if(Plan::valid_path(player.position(), dest, world, player)) {
		player.move_move(dest, (target - dest).orientation(), finalVel);
	}else {
		Action::move(ca, world, player, dest, (target - dest).orientation());
	}
}

