#include "ai/tactic/shoot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pivot.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

#include "uicomponents/param.h"

#include <vector>
#include <iostream>

namespace {
	DoubleParam ALLOWANCE_FACTOR("shoot: scale goal view angle", 0.5, 0.1, 1.0);
	DoubleParam SHOOT_KICK_ANGLE("shoot: angle it can see before it should kick (degrees)", 5.0, 1.0, 20.0);
}

Shoot::Shoot(RefPtr<Player> player, RefPtr<World> world) : Tactic(player), the_world(world), forced(false), use_pivot(true), allow_dribble(true) {
}

void Shoot::tick() {
	const FriendlyTeam &friendly(the_world->friendly);
	const EnemyTeam &enemy(the_world->enemy);

	const std::pair<Point, double> bestshot = AIUtil::calc_best_shot(the_world, player, true, forced);
	
	if (forced) {
		allow_dribble = false;
	}

	if (AIUtil::has_ball(the_world, player)) {
		// This player has the ball.
		LOG_INFO("The player has the ball.");

		std::vector<Point> obstacles;
		for (size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get_player(i) == player) continue;
			obstacles.push_back(friendly[i]->position());
		}
		for (size_t i = 0; i < enemy.size(); ++i) {
			obstacles.push_back(enemy[i]->position());
		}

		const Point diff = bestshot.first - player->position();
		const double targetori = diff.orientation();

		// calculate where to aim
		Move move_tactic(player, the_world);
		move_tactic.set_orientation(targetori);

		// dribble if possible to
		if (allow_dribble && bestshot.second < degrees2radians(SHOOT_KICK_ANGLE) && player->dribble_distance() < Player::MAX_DRIBBLE_DIST) {
			move_tactic.set_position(bestshot.first);
		}

		const double anglediff = angle_diff(targetori, player->orientation());
		LOG_INFO(Glib::ustring::compose("target=%1,%2 tolerance=%3 off=%4", bestshot.first.x, bestshot.first.y, bestshot.second, anglediff));

		// check if the goal is within shooting range. if so, kick
		if (anglediff * 2 < bestshot.second * ALLOWANCE_FACTOR) {
			// kick realy really hard
			if (player->chicker_ready_time() == 0) {
				LOG_INFO(Glib::ustring::compose("%1 kick", player->name));
				player->kick(1.0);
			} else {
				LOG_DEBUG(Glib::ustring::compose("%1 chicker not ready", player->name));
			}
		} else {
			LOG_DEBUG(Glib::ustring::compose("%1 aiming", player->name));
		}

		move_tactic.set_flags(flags);
		move_tactic.tick();	
	} else if (use_pivot) {
		Pivot tactic(player, the_world);
		tactic.set_target(bestshot.first);
		tactic.set_flags(flags);
		tactic.tick();
	} else {
		Chase chase_tactic(player, the_world);
		chase_tactic.set_flags(flags);
		chase_tactic.tick();
	}
}

