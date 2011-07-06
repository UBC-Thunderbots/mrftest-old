#include "ai/hl/stp/tactic/pass_simple.h"
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
#include "ai/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Coordinate;

using AI::HL::STP::min_pass_dist;

namespace {

	struct PasserSimple : public Tactic {
			bool kick_attempted;

			// HYSTERISIS
			Player::CPtr target;

			PasserSimple(const World &world) : Tactic(world, true), kick_attempted(false) {
			}

			bool done() const {
				return player.is() && kick_attempted && player->autokick_fired();
			}

			void player_changed() {
				target = Evaluation::select_passee(world);
			}

			bool fail() const {
				if (!Evaluation::passee_suitable(world, target)) {
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
				if (!target.is()) {
					// should fail
					return;
				}

				kick_attempted = kick_attempted || Action::shoot_pass(world, player, target);
			}

			std::string description() const {
				return "passer-simple";
			}
	};

	struct PasseeSimple : public Tactic {
		const unsigned number;

		PasseeSimple(const World &world, unsigned number) : Tactic(world, false), number(number) {
		}

		Player::Ptr select(const std::set<Player::Ptr> &players) const {
			// hysterysis.
			if (player.is() && players.count(player) && Evaluation::passee_suitable(world, player)) {
				return player;
			}
			for (auto it = players.begin(); it != players.end(); ++it) {
				if (Evaluation::passee_suitable(world, *it)) {
					return *it;
				}
			}
			// return a random player..
			return *players.begin();
		}

		void execute() {
			if (Evaluation::passee_suitable(world, player)) {
				Action::move(world, player, player->position());
			} else {
				// move to a suitable position
				auto dest = AI::HL::STP::Evaluation::offense_positions(world);
				Action::move(world, player, dest[number]);
				player->prio(AI::Flags::MovePrio::LOW);
			}
		}

		std::string description() const {
			return "passee-simple";
		}
	};

	Point grab_ball_dest(const World& world, Player::CPtr player) {
		Player::CPtr baller = Evaluation::calc_friendly_baller(world);
		if (!baller.is()) {
			return world.ball().position();
		}

		Point ball_vel = Action::pass_speed * Point::of_angle(baller->orientation());
		Point dest;
		if (AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), ball_vel, player->position(), dest)) {
			return dest;
		}
		return world.ball().position();
	}

	struct FollowBaller : public Tactic {

		FollowBaller(const World &world) : Tactic(world, false) {
		}

		Player::Ptr select(const std::set<Player::Ptr> &players) const {
			Player::Ptr best;
			double min_dist = 1e99;
			for (auto it = players.begin(); it != players.end(); ++it) {
				Point dest = grab_ball_dest(world, *it);
				if (!best.is() || min_dist > (dest - (*it)->position()).len()) {
					min_dist = (dest - (*it)->position()).len();
					best = *it;
				}
			}
			return best;
		}

		void execute() {
			if (Evaluation::passee_suitable(world, player)) {
				Action::move(world, player, player->position());
			} else {
				Point dest = grab_ball_dest(world, player);
				Action::move(world, player, dest);
				player->prio(AI::Flags::MovePrio::LOW);
			}
		}

		std::string description() const {
			return "follow-passer";
		}
	};

}

Tactic::Ptr AI::HL::STP::Tactic::passer_simple(const World &world) {
	const Tactic::Ptr p(new PasserSimple(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_simple(const World &world, unsigned number) {
	const Tactic::Ptr p(new PasseeSimple(world, number));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::follow_baller(const World &world) {
	const Tactic::Ptr p(new FollowBaller(world));
	return p;
}

