#include "ai/hl/stp/tactic/pass_ray.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
using AI::HL::STP::min_pass_dist;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {

	DoubleParam small_pass_ray_angle("Small ray shoot rotation (degrees)", "STP/PassRay", 20, 0, 180);

	struct PasserRay : public Tactic {
		bool kick_attempted;
		Player::CPtr target;

		// HYSTERESIS
		double ori_fix;

		PasserRay(const World &world) : Tactic(world, true), kick_attempted(false) {
		}

		bool done() const {
			return player.is() && kick_attempted && player->autokick_fired();
		}

		void player_changed() {
			ori_fix = Evaluation::best_shoot_ray(world, player).second;
		}

		bool fail() const {
			double ori = Evaluation::best_shoot_ray(world, player).second;
			if (angle_diff(ori, ori_fix) > degrees2radians(small_pass_ray_angle)) {
				return true;
			}
			return false;
		}

		Player::Ptr select(const std::set<Player::Ptr> &players) const {
			// if a player attempted to shoot, keep the player
			if (kick_attempted && players.count(player)) {
				return player;
			}
			return select_baller(world, players);
		}

		void execute() {
			double ori = Evaluation::best_shoot_ray(world, player).second;

			Point target = player->position() + 10 * Point::of_angle(ori);
			if (Action::shoot_target(world, player, target, Action::pass_speed)) {
				kick_attempted = true;
			}
			player->flags(0);
		}

		std::string description() const {
			return "passer-ray";
		}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_ray(const World &world) {
	const Tactic::Ptr p(new PasserRay(world));
	return p;
}

