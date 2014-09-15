#include "ai/hl/stp/action/cshoot.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/flags.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "geom/rect.h"
#include "util/param.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>
#include "ai/hl/stp/param.h"

using namespace AI::HL::STP;

bool AI::HL::STP::Action::cshoot_target(World world, Player player, const Point target, double velocity) {
	// Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot_target(world, player, target);
	intercept(player, target);

	// if (shoot_data.can_shoot) {
	if (!Evaluation::player_within_angle_thresh(player, target, passer_angle_threshold)) {
		return false;
	}

	// angle is right but chicker not ready, ram the ball and get closer to target
	if (!player.chicker_ready()) {
		LOG_INFO(u8"chicker not ready");
		// angle is right but chicker not ready, ram the ball and get closer to target, only use in normal play
		if (world.playtype() == AI::Common::PlayType::PLAY) {
			const Point diff = world.ball().position() - player.position();
			ram(world, player, target, diff * 8.0);
		}
		return false;
	}
//make sure velocity is updated for 2013
	LOG_INFO(u8"autokick");
	player.autokick(velocity);
	return true;
}
