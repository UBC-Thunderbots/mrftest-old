#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>
#include "ai/hl/stp/param.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
}

bool AI::HL::STP::Action::chip_target(World world, Player player, const Point target, double power) {
	// grab the ball in case we don't have it or lost it
	intercept_pivot(world, player, target);

	if (!Evaluation::player_within_angle_thresh(player, target, passer_angle_threshold)) {
		return false;
	}

	// angle is right but chicker not ready, ram the ball and get closer to target
	if (!player.chicker_ready()) {
		LOG_INFO("chicker not ready");
		// angle is right but chicker not ready, ram the ball and get closer to target, only use in normal play
		if (world.playtype() == AI::Common::PlayType::PLAY) {
			const Point diff = world.ball().position() - player.position();
			ram(world, player, target, diff * FAST);
		}
		return false;
	}

	LOG_INFO("autochip");
	player.autochip(power);
	return true;
}

