#include "ai/hl/stp/evaluation/ball_threat.h"
#include "geom/util.h"
#include "ai/hl/util.h"

#include <algorithm>

using namespace AI::HL::W;
using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	DoubleParam steal_threshold("Distance of ball from enemy robot to activate steal mechanism (robot radius)", 1.1, 0.8, 5.0);

	DoubleParam negligible_velocity("decides at which speed goalie should ignore direction of ball ", 0.05, 1e-4, 1.0);

}

BallThreat AI::HL::STP::Evaluation::evaluate_ball_threat(const World &world) {
	BallThreat ball_threat;

	ball_threat.enemies = AI::HL::Util::get_robots(world.enemy_team());
	ball_threat.activate_steal = false;

	if (ball_threat.enemies.size() > 0) {
		std::sort(ball_threat.enemies.begin(), ball_threat.enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));

		ball_threat.threat = ball_threat.enemies[0];

		ball_threat.threat_distance = (ball_threat.threat->position() - world.ball().position()).len();

		// steal mechanism activation
		if ((ball_threat.threat->position() - world.ball().position()).len() * Robot::MAX_RADIUS < steal_threshold) {
			ball_threat.activate_steal = true;
		}
	}

	return ball_threat;
}

bool ball_on_net(const AI::HL::W::World &world) {

	if (world.ball().velocity().lensq() < negligible_velocity || world.ball().velocity().x > 0 ) {
		return false;
	}

	Point a =  world.field().friendly_goal_boundary().first;
	Point b = world.field().friendly_goal_boundary().second;
	Point c = world.ball().position();
	Point d =  world.ball().position() +  world.ball().velocity();

	if( unique_line_intersect( a, b, c, d ) ){
		Point inter = line_intersect(a, b, c, d);
		return inter.y <= std::max(a.y, b.y) && inter.y >= std::min(a.y, b.y);
	}
	return false;
}


Point goalie_shot_block(const AI::HL::W::World &world) {
	if(world.friendly_team().size()<=0 || ! ball_on_net(world)) {
		return Point(0,0);
	}

	Point goalie_pos = world.friendly_team().get(0)->position();

	Point a =  world.field().friendly_goal_boundary().first;
	Point b = world.field().friendly_goal_boundary().second;

	Point c = world.ball().position();
	Point d =  world.ball().position() +  world.ball().velocity();
	Point inter = line_intersect(a, b, c, d);

	return closest_lineseg_point( goalie_pos, c, inter) ;
}

