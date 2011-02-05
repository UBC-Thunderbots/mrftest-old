#include "ai/hl/stp/evaluate/ball_threat.h"
#include "ai/hl/util.h"

#include <algorithm>

using namespace AI::HL::W;
using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	DoubleParam steal_threshold("Distance of ball from enemy robot to activate steal mechanism (robot radius)", 1.1, 0.8, 5.0);
}

const BallThreat AI::HL::STP::Evaluation::evaluate_ball_threat(World &world) {
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

