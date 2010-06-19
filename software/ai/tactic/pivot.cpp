#include "ai/tactic/pivot.h"
#include "geom/angle.h"
#include "ai/flags.h"
#include "ai/tactic/chase.h"
#include "ai/util.h"
#include "ai/navigator/robot_navigator.h"
#include "util/dprint.h"

#include "uicomponents/param.h"

#include <iostream>
#include <cmath>

namespace {

	const double PIVOT_ORI_CLOSE = 10.0 / 180.0 * M_PI;
	const double PIVOT_ANGLE = M_PI / 2;
	double_param PIVOT_DIST("Pivot Distance", 0.08, 0.1, 0.2);

}

pivot::pivot(player::ptr player, world::ptr world) : tactic(player), the_world(world), target(world->field().enemy_goal()), get_ball_(true) {
}

void pivot::tick() {

	// use robot navigator instead of storing move tactic.
	// the reason is that we can't unset flags.
	robot_navigator navi(the_player, the_world);

	const point ballpos = the_world->ball()->position();

	// invalid target
	if ((target - ballpos).len() < ai_util::POS_EPS) {
		LOG_WARN("ball is already in destination");
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	// do all relative to the ball
	const point tobehind = (ballpos - target).norm(); // points behind
	const point toplayer = (the_player->position() - ballpos).norm(); // points to the player
	const double anglediff = angle_diff(tobehind.orientation(), toplayer.orientation()); // angle difference

	point wantdest = ballpos;

	// we can do something to the ball now!
	if (get_ball_ && ((tobehind.dot(toplayer) > 0 && anglediff < PIVOT_ORI_CLOSE) || ai_util::has_ball(the_world, the_player))) {
		const point playertotarget = target - the_player->position();
		if (ai_util::has_ball(the_world, the_player)) {
			navi.set_position(target);
			navi.set_orientation(playertotarget.orientation());
		} else {
			navi.set_position(ballpos);
		}
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	// now we have to know which direction to take.. left or right
	const double rotangle = std::min(PIVOT_ANGLE, anglediff);
	const double pivotdist = PIVOT_DIST + robot::MAX_RADIUS + ball::RADIUS;
	if (tobehind.cross(toplayer) > 0) {
		// left
		wantdest = ballpos + toplayer.rotate(-rotangle) * pivotdist;
	} else {
		// right
		wantdest = ballpos + toplayer.rotate(rotangle) * pivotdist;
	}

	navi.set_position(wantdest);
	navi.set_flags(flags);
	navi.tick();
}

